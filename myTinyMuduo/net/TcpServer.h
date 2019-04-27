//
// Created by john on 4/23/19.
//

#ifndef MY_TINYMUDUO_NET_TCPSERVER_H
#define MY_TINYMUDUO_NET_TCPSERVER_H

#include "base/Atomic.h"
#include "base/Types.h"
#include "net/TcpConnection.h"

#include <map>

namespace muduo{
    namespace net{
        class Acceptor;
        class EventLoop;
        class EventLoopThreadPool;

        class TcpServer:noncopyable{
        public:
            typedef std::function<void(EventLoop*)>ThreadInitCallback;
            enum Option{kNoReusePort, kReusePort,};

            TcpServer(EventLoop* loop,const InetAddress& listenAddr,const string& nameArg,Option option = kNoReusePort);
            ~TcpServer();

            //返回类的一些属性
            const string& ipPort()const{return ipPort_;};
            const string& name()const{return name_;}
            EventLoop* getLoop()const{return loop_;}


            //设置线程数
            //一直在线程的事件循环中接受新连接
            //必须在start前调用
            //0（默认值）：不额外创建线程，所有的I/O都在主线程里
            //1        ： 只额外创建一个线程，所有的I/O分配给这个线程里
            //n（n>1）  ： 创建多个线程，I/O一般通过轮转调度的方式分配给这些线程
            void setThreadNum(int numThreads);
            void setThreadInitCallback(const ThreadInitCallback& cb){ threadInitCallback_ = cb;}
            std::shared_ptr<EventLoopThreadPool> threadPool(){ return threadPool_;}

            void start();

            void setConnectionCallback(const ConnectionCallback& cb){ connectionCallback_ = cb; }

            void setMessageCallback(const MessageCallback& cb){ messageCallback_ = cb;}

            void setWriteCompleteCallback(const WriteCompleteCallback& cb){ writeCompleteCallback_ = cb;}

        private:
            void newConnection(int sockfd,const InetAddress& peerAddr);
            void removeConnection(const TcpConnectionPtr& conn);
            void removeConnectionInLoop(const TcpConnectionPtr& conn);

            typedef std::map<string, TcpConnectionPtr>ConnectionMap;

            EventLoop*      loop_;
            const string    ipPort_;
            const string    name_;
            std::unique_ptr<Acceptor>               acceptor_;
            std::shared_ptr<EventLoopThreadPool>    threadPool_;
            ConnectionCallback                      connectionCallback_;
            MessageCallback                         messageCallback_;
            WriteCompleteCallback                   writeCompleteCallback_;
            ThreadInitCallback                      threadInitCallback_;
            AtomicInt32                             started_;
            // always in loop thread
            int                                     nextConnId_;
            ConnectionMap                           connections_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_TCPSERVER_H
