//
// Created by john on 4/18/19.
//

#ifndef MY_TINYMUDUO_NET_SOCKETSOPS_H
#define MY_TINYMUDUO_NET_SOCKETSOPS_H

#include "arpa/inet.h"

namespace muduo{
    namespace net{
        namespace sockets{
            //创建1个非阻塞的套接字描述符,创建失败直接abort
            int creatNonblockingOrDie(sa_family_t family);
            //封装底层connect,bind,listen,accept,close,shutdown
            int connect(int sockfd,const struct sockaddr* addr);
            void bindOrDie(int sockfd, const struct sockaddr* addr);
            void listenOrDie(int sockfd);
            int  accept(int sockfd, struct sockaddr_in6* addr);
            void close(int sockfd);
            void shutdownWrite(int sockfd);

            ssize_t read(int sockfd, void *buf, size_t count);
            ssize_t write(int sockfd, const void *buf, size_t count);
            //从多个缓冲读
            ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);

            //IP转换和端口转换
            void toIpPort(char* buf, size_t size,const struct sockaddr* addr);
            void toIp(char* buf, size_t size, const struct sockaddr* addr);
            void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
            void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);

            int getSocketError(int sockfd);
            //sockaddr和sockaddr_in之间的转换
            const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
            const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
            struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);
            const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
            const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

            struct sockaddr_in6 getLocalAddr(int sockfd);
            struct sockaddr_in6 getPeerAddr(int sockfd);
            bool isSelfConnect(int sockfd);
        }
    }
}

#endif //MY_TINYMUDUO_NET_SOCKETSOPS_H
