//
// Created by john on 4/22/19.
//

#include "net/TcpConnection.h"

#include "base/Logging.h"
#include "base/WeakCallback.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "net/SocketsOps.h"

#include <errno.h>

using namespace muduo;
using namespace muduo::net;

//默认建立/断开连接时的回调函数
void muduo::net::defaultConnectionCallback(const TcpConnectionPtr &conn) {

    LOG_TRACE<<conn->localAddress().toIpPort()<<"->"
             <<conn->peerAddress().toIpPort()<<" is "
             <<(conn->connected()?"UP":"DOWN"); //根据TcpConnection的状态，若为kConnected：“up”
                                                                       //若为kDisConnectted：“down”
}

//默认接收数据回调函数，忽略读到的数据
void muduo::net::defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp receiveTime) {
    buffer->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
                : loop_(CHECK_NOTNULL(loop)),
                  name_(name),
                  state_(kConnecting),
                  reading_(true),
                  socket_(new Socket(sockfd)),
                  channel_(new Channel(loop,sockfd)),
                  localAddr_(localAddr),
                  peerAddr_(peerAddr),
                  highWaterMark_(64*1024*1024)
{
    //设置channel的回调函数（channel将调用TcpConnection的handle）
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError,this));
    LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
              << " fd=" << sockfd;
    //TCP内嵌心跳包，若TCP连接长时间没有数据传输，
    //则服务端向客户端（多次）发送一个keepalive包，若客户端一直没有回应
    //则服务端认为客户端已断开连接
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG << "TcpConnection::dtor[" << name_<< "] at " << this
              <<" fd="<<channel_->fd()<<" state="<<stateToString();
    assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const {
    return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const {
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf,sizeof(buf));
    return buf;
}

void TcpConnection::send(const void *data, int len) {
    send(StringPiece(static_cast<const char*>(data),len));
}

void TcpConnection::send(const StringPiece &message) {
    if(state_ == kConnected){
        if(loop_->isInloopThread()){
            //在TcpConnection所属的EventLoop所在的线程调用
            sendInLoop(message);
        }else{
            //若被其他线程调用，定义函数指针fp指向sendInLoop成员函数(函数重载显式定义函数指针)
            void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
            //放入本EventLoop的任务队列等待执行
            loop_->runInLoop(std::bind(fp,this,message.as_string()));
        }
    }
}
//类似于send(const StringPiece&)
void TcpConnection::send(Buffer *buf) {
    if(state_ == kConnected){
        if(loop_->isInloopThread()){
            sendInLoop(buf->peek(),buf->readableBytes());
            buf->retrieveAll();
        }else{
            void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp,this,buf->retrieveAllAsString()));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece &message) {
    sendInLoop(message.data(),message.size());
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
    //不允许跨线程
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if(state_ == kDisconnected){
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    //如果TcpConnection写缓冲区内没有数据，直接将数据写入套接字（写入系统缓冲区）
    if(!channel_->isWriting()&&outputBuffer_.readableBytes() == 0){
        nwrote = sockets::write(channel_->fd(),data,len);
        if(nwrote >= 0){
            remaining = len - nwrote;
            //如果数据一次发完了，将发送完成回调函数放到任务队列中等待执行（现在执行？）
            //writeCompleteCallback_包含本TcpConnection对象的shared_ptr指针，这会延长其生命期
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
            }
        }else{
            //write出错令nwrote = 0，表示一个也没写到fd
            nwrote = 0;
            //EWOULDBLOCK表示系统缓冲区满（非阻塞）
            if(errno != EWOULDBLOCK){
                LOG_SYSERR<<"TcpConnection::sendInLoop";
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }
    assert(remaining <= len);
    //上面的write()没有出错，并且还有一部分数据未写入fd；或者原来发送缓冲区里还有数据要发
    if(!faultError && remaining > 0){
        //发送缓冲区还有多少数据要发
        size_t oldLen = outputBuffer_.readableBytes();
        //如果缓存中待写的数据过多，达到高水位标记,并且缓冲里原来的数据在高水位以下
        //highWaterMarkCallback_存在，将其放入任务队列中等待执行
        if(oldLen +remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_){
            loop_->queueInLoop(std::bind(highWaterMarkCallback_,shared_from_this(),oldLen+remaining));
        }
        //将还未写入的数据添加到TcpConnection写缓冲里
        outputBuffer_.append(static_cast<const char*>(data)+nwrote,remaining);
        //开启Channel对象对写事件的监听
        if(!channel_->isWriting()){
            channel_->enableWritting();
        }
    }
}

void TcpConnection::shutdown() {
    if(state_ == kConnected){
        //设置状态，半关闭
        setState(kDisconnecting);
                                                                //shared_from_this（）？？
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}

//关闭写
void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    //相应channel是否停止对写事件的监听
    if(!channel_->isWriting()){
        //套接字关闭写
        socket_->shutdownWrite();
    }
}


void TcpConnection::forceClose() {
    if(state_ == kConnected||state_ == kDisconnecting){
        setState(kDisconnecting);
        //在任务对象forceCloseInLoop执行之前，TcpConnection对象不会销毁
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop,shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds) {
    if(state_ == kConnected||state_ == kDisconnecting){
        setState(kDisconnecting);
        //调用forceClose而不是forceCloseInLoop可以避免争用
        //添加到任务里的是弱回调对象，若开始执行任务的时候TcpConnection对象已销毁则什么也不做
        loop_->runAfter(seconds,makeWeakCallback(shared_from_this(),&TcpConnection::forceClose));
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if(state_ == kConnected||state_ == kDisconnecting){
        //handleClose真正处理关闭连接
        handleClose();
    }
}

const char* TcpConnection::stateToString() const {
    switch (state_)
    {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

//关闭nagle
void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::starRead() {
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop,this));
}

void TcpConnection::startReadInLoop() {
    loop_->assertInLoopThread();
    if(!reading_ || !channel_->isReading()){
        //让Channel对象开启读事件监听
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead() {
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop,this));
}

void TcpConnection::stopReadInLoop() {
    loop_->assertInLoopThread();
    if(reading_||channel_->isReading()){
        channel_->disableReading();
        reading_ = false;
    }
}

void TcpConnection::connectEstablished(){
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    //让channel里弱指针成员指向该TcpConnction对象，则channel可以测试
    // TcpConnection对象是否还存活，也可以延长TcpConnection的生命期
    channel_->tie(shared_from_this());

    channel_->enableReading();
    //调用建立连接回调函数
    connetionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if(state_ == kConnected){
        setState(kDisconnected);
        channel_->disableAll();
        connetionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    //接收数据存到接收缓冲区
    ssize_t n=inputBuffer_.readFd(channel_->fd(),&savedErrno);
    if(n>0){
        //接收到了数据，调用messageCallback_进行数据处理
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }else if(n == 0){
        //对shut_down的响应接收到0个数据表明对端以关闭连接
        handleClose();
    }else{
        errno = savedErrno;
        LOG_SYSERR<<"TcpConnection::hanleRead";
        handleError();
    }
}

void TcpConnection::handleWrite(){
    loop_->assertInLoopThread();
    if(channel_->isWriting()){
        //将发送缓冲区的数据，写入套接字
        ssize_t n = sockets::write(channel_->fd(),outputBuffer_.peek(),outputBuffer_.readableBytes());

        if(n>0){
            outputBuffer_.retrieve(n);
            //如果一次发完（发多少算多少，未发的数据留在TcpConnection的写缓冲区，等待下次可写事件）
            if(outputBuffer_.readableBytes() == 0){
                //关闭channel对写事件的监听
                channel_->disableWriting();
                if(writeCompleteCallback_){
                    //将发送完成回调函数添加到任务队列
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                //发送完成检查一下状态，若连接关闭则关闭其写端
                //因为在发送缓冲区还有数据等待发送时，不会调用shutdownWrite
                //所以最后在把发送缓冲区的数据发送完后，再次调用shutdownInLoop()
                if(state_ == kDisconnected){
                    shutdownInLoop();
                }
            }
        }else{
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    }else{
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}

void TcpConnection::handleClose(){
    loop_->assertInLoopThread();
    LOG_TRACE <<"fd = "<<channel_->fd()<<" state = "<<stateToString();
    assert(state_ == kConnected || state_ == kDisconnecting);

    //状态改为断开连接，并关闭channel_对所有事件的监听
    setState(kDisconnected);
    channel_->disableAll();

    //让该TcpConnection对象依然存活直到connetionCallback_，closeCallback_调用完成
    TcpConnectionPtr guardThis(shared_from_this());
    connetionCallback_(guardThis);
    closeCallback_(guardThis);
}

void TcpConnection::handleError() {
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR<<"TcpConnection::handleError ["<<name_
             <<"] -SO_ERROR = "<<err<<" "<<strerror_tl(err);
}












