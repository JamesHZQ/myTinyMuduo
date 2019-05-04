//
// Created by john on 4/21/19.
//

#ifndef MY_TINYMUDUO_NET_TCPCONNECTION_H
#define MY_TINYMUDUO_NET_TCPCONNECTION_H

#include "base/noncopyable.h"
#include "base/StringPiece.h"
#include "base/Types.h"
#include "net/Callbacks.h"
#include "net/Buffer.h"
#include "net/InetAddress.h"

#include <memory>
#include <boost/any.hpp>

struct tcp_info;

namespace muduo{
    namespace net{
        class Channel;
        class EventLoop;
        class Socket;

        class TcpConnection: noncopyable,
                             public std::enable_shared_from_this<TcpConnection>
        {
        public:
            TcpConnection(EventLoop* loop,const string& name,int sockfd,const InetAddress& localAddr,const InetAddress& peerAddr);
            ~TcpConnection();

            //返回内部成员信息
            EventLoop* getLoop()const{return loop_;}
            const string& name()const{return name_;}
            const InetAddress& localAddress()const{return localAddr_;}
            const InetAddress& peerAddress()const{return peerAddr_;}
            bool connected()const{ return state_ == kConnected;}
            bool disconnected()const {return state_ == kDisconnected;}
            bool getTcpInfo(struct tcp_info*)const;
            string getTcpInfoString()const;

            //发送数据send
            //void send(string&& );
            void send(const void* ,int );
            void send(const StringPiece& );
            //void send(Buffer&& );
            void send(Buffer*);

            //半关闭、关闭、延时关闭
            void shutdown();
            void forceClose();
            void forceCloseWithDelay(double seconds);

            //关闭nagle算法
            void setTcpNoDelay(bool on);

            //接收数据read
            void starRead();
            void stopRead();
            bool isReading()const{return reading_;}

//            void setContext(const boost::any& context){
//                context_ = context;
//            }
//            const boost::any& getMutableContex(){
//                return &context_;
//            }

            //5个可供用户设置的回调函数
            //连接建立/断开后调用cb
            void setConnectionCallback(const ConnectionCallback& cb){
                connetionCallback_ = cb;
            }
            //收到数据后调用cb
            void setMessageCallback(const MessageCallback& cb){
                messageCallback_ = cb;
            }
            //发送缓冲区清空（全部发送）调用cb
            void setWriteCompleteCallback(const WriteCompleteCallback& cb){
                writeCompleteCallback_ = cb;
            }
            //发送缓冲区数据太多，达到高水位标记调用cb
            void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,size_t highWaterMark){
                highWaterMarkCallback_ = cb;
                highWaterMark_ = highWaterMark;
            }
            //连接关闭后调用cb
            void setCloseCallback(const CloseCallback& cb){
                closeCallback_ = cb;
            }

            //分别返回TcpConnection内部的读/写缓冲区
            Buffer* inputBuffer(){
                return &inputBuffer_;
            }
            Buffer* outputBuffer(){
                return &outputBuffer_;
            }

            void connectEstablished();
            void connectDestroyed();

        private:
            enum StateE{kDisconnected,kConnecting, kConnected, kDisconnecting};
            void handleRead(Timestamp receiveTime);
            void handleWrite();
            void handleClose();
            void handleError();

            void sendInLoop(const StringPiece& );
            void sendInLoop(const void* ,size_t );
            void shutdownInLoop();
            void forceCloseInLoop();
            void setState(StateE s){state_ = s;}
            const char* stateToString()const;
            void startReadInLoop();
            void stopReadInLoop();

            EventLoop*      loop_;
            const string    name_;
            StateE          state_;
            bool            reading_;
            //TcpConnection对象拥有Socket和Channel
            std::unique_ptr<Socket>     socket_;
            std::unique_ptr<Channel>    channel_;
            const InetAddress       localAddr_;
            const InetAddress       peerAddr_;
            ConnectionCallback      connetionCallback_;
            MessageCallback         messageCallback_;
            WriteCompleteCallback   writeCompleteCallback_;
            HighWaterMarkCallback   highWaterMarkCallback_;
            CloseCallback           closeCallback_;
            size_t  highWaterMark_;
            Buffer  inputBuffer_;
            Buffer  outputBuffer_;
//            boost::any context_;
        };
        typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    }
}

#endif //MY_TINYMUDUO_NET_TCPCONNECTION_H




























