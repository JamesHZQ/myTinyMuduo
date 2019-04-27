//
// Created by john on 4/21/19.
//

#include "net/Connector.h"

#include "base/Logging.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/SocketsOps.h"

#include <errno.h>

using namespace muduo;
using namespace muduo::net;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
    LOG_DEBUG<<"ctor["<<this<<"]";
}

Connector::~Connector() {
    LOG_DEBUG << "dtor[" << this << "]";
    //channel_是unique_ptr类型，Connector对象析构，channel_指向的Channel对象也析构
    assert(!channel_);
}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop,this));
}

void Connector::startInLoop() {
    //不能跨线程
    loop_->assertInLoopThread();
    //Connector处于断开连接的状态，发起连接
    assert(state_ == kDisconnected);
    if(connect_){
        connect();
    } else{
        LOG_DEBUG<<"do not connect";
    }
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop,this));
}

void Connector::stopInLoop() {
    loop_->assertInLoopThread();
    if(state_ == kConnecting){
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect() {
    int sockfd = sockets::creatNonblockingOrDie(serverAddr_.family());
    //发起连接请求
    int ret = sockets::connect(sockfd,serverAddr_.getSockAddr());
    int savedErrno = (ret==0)?0:errno;
    switch (savedErrno){
        case 0:
        case EINPROGRESS:       //非阻塞模式下，没有立即建立连接
        case EINTR:             //系统中断
        case EISCONN:           //重复连接请求
            connecting(sockfd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:      //拒绝连接
        case ENETUNREACH:       // 网络不可达
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:      //套接字错误
            LOG_SYSERR<<"connect error in Connector::startInLoop "<<savedErrno;
            sockets::close(sockfd);
            break;
        default:
            LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}

void Connector::restart(){
    //不能跨线程
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::connecting(int sockfd) {
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_,sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite,this));
    channel_->setErrorCallback(std::bind(&Connector::handleError,this));

    channel_->enableWritting();
}
//清除Connector的Channel对象，将已连接套接字传给TcpConneciton，
// TcpConneciton创建属于自己的对应已连接套接字的channel，并会添加到poll描述符集
//如果不清除Connector的Channel对象，会发生冲突
//这和Acceptor是不同的，因为Acceptor一直使用自己的监听套接字，Tcp连接建立后，将新的
// 连接套接字交给TcpConneciton管理（Acceptor不创建对应新的监听套接字的channel，
// 而是交给TcpConneciton来创建）
int Connector::removeAndResetChannel() {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    //事件循环中不能直接移除channel（可能正处于handleEvent期）
    loop_->queueInLoop(std::bind(&Connector::resetChannel,this));
    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

void Connector::handleWrite() {
    LOG_TRACE<<"Connector::handleWrite "<<state_;

    if(state_ == kConnecting){
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if(err){
            LOG_WARN<<"Connector::handleWrite - SO_ERROR = "<<err<<" "<<strerror_tl(err);
            retry(sockfd);
        }else if(sockets::isSelfConnect(sockfd)){
            LOG_WARN<<"Connector::handleWrite - Self connect";
            retry(sockfd);
        }else{
            setState(kConnected);
            if(connect_){
                newConnectionCallback_(sockfd);
            }else{
                sockets::close(sockfd);
            }
        }
    }else{
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError(){
    LOG_ERROR<<"Connector::handleError state"<<state_;
    if(state_ == kConnecting){
        int sockfd=removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
        retry(sockfd);
    }

}

void Connector::retry(int sockfd){
    //用于发起连接的套接字是一次性的，连接创建失败后
    //再次发起连接需要创建新的套接字，关闭原来旧的套接字
    sockets::close(sockfd);
    setState(kDisconnected);
    if(connect_){
        LOG_INFO<<"Connector::retry - Retry connecting " << serverAddr_.toIpPort()<<" in "<<retryDelayMs_<<" milliseconds. ";
        //延时retryDelayMs_/1000.0，再次发起连接
        loop_->runAfter(retryDelayMs_/1000.0,std::bind(&Connector::startInLoop,shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_*2,kMaxRetryDelayMs);
    }
    else{
        LOG_DEBUG<<"do not connect";
    }
}

















