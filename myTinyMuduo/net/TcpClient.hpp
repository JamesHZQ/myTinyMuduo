//
// Created by john on 4/27/19.
//

#ifndef MY_TINYMUDUO_NET_TCPCLIENT_HPP
#define MY_TINYMUDUO_NET_TCPCLIENT_HPP

#include "base/Mutex.hpp"
#include "net/TcpConnection.hpp"

namespace muduo{
    namespace net{
        class Connector;
        typedef std::shared_ptr<Connector> ConnectorPtr;
        class TcpClient:noncopyable{
        public:
            TcpClient(EventLoop* loop,const InetAddress,const std::string& nameArg);
            ~TcpClient();

            void connect();
            void disconnect();
            void stop();

            TcpConnectionPtr connection()const{
                MutexLockGuard lock(mutex_);
                return connection_;
            }

            EventLoop* getLoop()const{ return loop_;}
            bool retry()const{ return retry_;};
            void enableRetry(){retry_ = true;};
            const std::string& name()const{return name_;}

            void setConnectionCallback(ConnectionCallback cb){connectionCallback_ = std::move(cb);}
            void setMessageCallback(MessageCallback cb){messageCallback_ = std::move(cb);}
            void setWriteCompleteCallback(WriteCompleteCallback cb){writeCompleteCallback_ = std::move(cb);}


            private:
                void newConnection(int sockfd);
                void removeConnection(const TcpConnectionPtr& conn);

                EventLoop*              loop_;
                ConnectorPtr            connector_;
                const std::string            name_;

                ConnectionCallback      connectionCallback_;
                MessageCallback         messageCallback_;
                WriteCompleteCallback   writeCompleteCallback_;

                bool                    retry_;
                bool                    connect_;
                int                     nextConnId_;
                mutable MutexLock       mutex_;
                TcpConnectionPtr        connection_;

        };
    }
}



#endif //MY_TINYMUDUO_NET_TCPCLIENT_H
