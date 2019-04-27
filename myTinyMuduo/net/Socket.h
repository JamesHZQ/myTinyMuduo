//
// Created by john on 4/18/19.
//

#ifndef MY_TINYMUDUO_NET_SOCKET_H
#define MY_TINYMUDUO_NET_SOCKET_H

#include "base/noncopyable.h"

struct tcp_info;

namespace muduo{
    namespace net{
        class InetAddress;
        class Socket:noncopyable{
        public:
            explicit Socket(int sockfd)
                : sockfd_(sockfd)
            {}

            ~Socket();

            int fd()const{ return sockfd_;}

            bool getTcpInfo(struct tcp_info*)const;
            bool getTcpInfoString(char* buf,int len)const;

            void bindAddress(const InetAddress& localaddr);
            void listen();

            int accept(InetAddress* peeraddr);

            void shutdownWrite();

            //（开启或关闭nagle算法）
            void setTcpNoDelay(bool on);

            void setReuseAddr(bool on);

            void setReusePort(bool on);

            void setKeepAlive(bool on);
        private:
            const int sockfd_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_SOCKET_H
