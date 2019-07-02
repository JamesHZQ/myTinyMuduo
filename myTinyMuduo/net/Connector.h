//
// Created by john on 4/21/19.
//

#ifndef MY_TINYMUDUO_NET_CONNECTOR_H
#define MY_TINYMUDUO_NET_CONNECTOR_H

#include "base/noncopyable.h"
#include "net/InetAddress.h"

#include <functional>
#include <memory>

namespace muduo{
    namespace net{
        class Channel;
        class EventLoop;

        class Connector:noncopyable,
                        public std::enable_shared_from_this<Connector>
        {
        public:
            typedef std::function<void (int)> NewConnectionCallback;

            Connector(EventLoop* loop, const InetAddress& serverAddr);
            ~Connector();

            void setNewConnectionCallback(const NewConnectionCallback& cb){
                newConnectionCallback_ = cb;
            }

            void start();        //可以跨线程
            void restart();     //不能跨线程
            void stop();        //可以跨线程

            const InetAddress& serverAddress()const{return serverAddr_;}

        private:
            enum States{kDisconnected,kConnecting,kConnected};
            static const int kMaxRetryDelayMs = 30*1000;
            static const int kInitRetryDelayMs = 500;

            void setState(States s){state_ = s;}
            void startInLoop();
            void stopInLoop();
            void connect();
            void connecting(int sockfd);
            void handleWrite();
            void handleError();
            void retry(int sockfd);
            int removeAndResetChannel();
            void resetChannel();

            EventLoop*                  loop_;
            InetAddress                 serverAddr_;
            bool                        connect_;
            States                      state_;
            std::unique_ptr<Channel>    channel_;
            NewConnectionCallback       newConnectionCallback_;
            int                         retryDelayMs_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_CONNECTOR_H
