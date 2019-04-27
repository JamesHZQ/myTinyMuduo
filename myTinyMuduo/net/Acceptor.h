//
// Created by john on 4/21/19.
//

#ifndef MY_TINYMUDUO_NET_ACCEPTOR_H
#define MY_TINYMUDUO_NET_ACCEPTOR_H

#include "net/Channel.h"
#include "net/Socket.h"

#include <functional>

namespace muduo{
    namespace net{
        class EventLoop;
        class InetAddress;

        class Acceptor:noncopyable{
        public:
            typedef std::function<void(int,const InetAddress&)> NewConnectionCallback;

            Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
            ~Acceptor();

            void setNewConnectionCallback(const NewConnectionCallback& cb){ newConnectionCallback_ = cb;}
            bool listenning()const{return listenning_;};
            void listen();
        private:
            void handleRead();

            EventLoop*  loop_;
            Socket      acceptSocket_;
            Channel     acceptChannel_;
            NewConnectionCallback newConnectionCallback_;
            bool        listenning_;
            int         idleFd_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_ACCEPTOR_H
