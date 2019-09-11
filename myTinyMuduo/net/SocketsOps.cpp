//
// Created by john on 4/18/19.
//

#include "net/SocketsOps.hpp"
#include "base/Logging.hpp"
#include "net/Endian.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;


const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in6 *addr) {
    return reinterpret_cast<const struct sockaddr*>(addr);
}
struct sockaddr* sockets::sockaddr_cast(struct sockaddr_in6* addr){
    return reinterpret_cast<struct sockaddr*>(addr);
}
const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr)
{
    return reinterpret_cast<const struct sockaddr*>(addr);
}
const struct sockaddr_in* sockets::sockaddr_in_cast(const struct sockaddr* addr){
    return reinterpret_cast<const struct sockaddr_in*>(addr);
}

const struct sockaddr_in6* sockets::sockaddr_in6_cast(const struct sockaddr* addr){
    return reinterpret_cast<const struct sockaddr_in6*>(addr);
}

int sockets::creatNonblockingOrDie(sa_family_t family) {
    //创建套接字的同时设置套接字描述的属性（TCP套接字+非阻塞+程序退出时自动关闭套接字）
    int sockfd = ::socket(family,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,IPPROTO_TCP);
    if(sockfd < 0){
        LOG_SYSFATAL<<"sockets::createNonblockingOrDie";
    }
    return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr *addr) {
    int ret = ::bind(sockfd,addr, static_cast<socklen_t >(sizeof(struct sockaddr_in6)));
    if(ret < 0){
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

void sockets::listenOrDie(int sockfd) {
    int ret = ::listen(sockfd,SOMAXCONN);
    if (ret < 0){
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

int sockets::accept(int sockfd, struct sockaddr_in6 *addr) {
    socklen_t addrlen = static_cast<socklen_t >(sizeof *addr);
#if VALGRIND || defined(NO_ACCEPT4)
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else
    //accept4可直接设置套接字描述符属性
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK|SOCK_CLOEXEC);
#endif
    if(connfd<0){
        int savedErrno = errno;
        LOG_SYSERR << "Socket::accept";
        switch(savedErrno){
            //达到开启文件描述符上限
            case EMFILE:
                errno = savedErrno;
                break;
            case EOPNOTSUPP:
                LOG_FATAL << "unexpected error of ::accept " << savedErrno;
                break;
            default:
                LOG_FATAL << "unknown error of ::accept " << savedErrno;
                break;
        }
    }
    return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr *addr) {
    return ::connect(sockfd,addr, static_cast<socklen_t >(sizeof(struct sockaddr_in6)));
}

ssize_t sockets::read(int sockfd, void *buf, size_t count) {
    return ::read(sockfd, buf, count);
}

ssize_t sockets::write(int sockfd, const void *buf, size_t count) {
    return ::write(sockfd,buf,count);
}

ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt) {
    return ::readv(sockfd,iov,iovcnt);
}

//关闭套接字
void sockets::close(int sockfd) {
    if(::close(sockfd) < 0){
        LOG_SYSFATAL<<"sockets::close";
    }
}

//仅关闭套接字写
void sockets::shutdownWrite(int sockfd) {
    if(::shutdown(sockfd, SHUT_WR) < 0){
        LOG_SYSERR<<"sockets::shutdownWrite";
    }
}

void sockets::toIp(char *buf, size_t size, const struct sockaddr *addr) {
    if(addr->sa_family == AF_INET){
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
        ::inet_ntop(AF_INET,&addr4->sin_addr, buf,static_cast<socklen_t>(size));
    }else if(addr->sa_family == AF_INET6){
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
        ::inet_ntop(AF_INET6,&addr6->sin6_addr,buf, static_cast<socklen_t>(size));
    }
}

void sockets::toIpPort(char *buf, size_t size, const struct sockaddr *addr) {
    toIp(buf,size,addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
    uint16_t port = sockets::networkToHost16(addr4->sin_port);
    assert(size > end);
    snprintf(buf+end,size-end,":%u",port);
}

void sockets::fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr) {
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if(::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0){
        LOG_SYSERR<<"sockets::fromIpPort";
    }
}

void sockets::fromIpPort(const char *ip, uint16_t port, struct sockaddr_in6 *addr) {
    addr->sin6_family = AF_INET6;
    addr->sin6_port = hostToNetwork16(port);
    if(::inet_pton(AF_INET6,ip,&addr->sin6_addr) <= 0){
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

//获取套接字错误
int sockets::getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t >(sizeof(optval));
    if(::getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&optval,&optlen)<0){
        return errno;
    }else{
        return optval;
    }
}

//获取本地地址
struct sockaddr_in6 sockets::getLocalAddr(int sockfd) {
    struct sockaddr_in6 localaddr;
    memset(&localaddr, 0, sizeof(localaddr));
    socklen_t addrlen = static_cast<socklen_t >(sizeof(localaddr));
    if(::getsockname(sockfd,sockaddr_cast(&localaddr),&addrlen) < 0){
        LOG_SYSERR<<"sockets::getLocalAddr";
    }
    return localaddr;
}

//获取对方地址
struct sockaddr_in6 sockets::getPeerAddr(int sockfd) {
    struct sockaddr_in6 peeraddr;
    memset(&peeraddr, 0, sizeof(peeraddr));
    socklen_t addrlen = static_cast<socklen_t >(sizeof(peeraddr));
    if(::getpeername(sockfd,sockaddr_cast(&peeraddr),&addrlen) < 0){
        LOG_SYSERR<<"sockets::getPeerAddr";
    }
    return peeraddr;
}

//判断是否是TCP自连接（连接双方IP，port是否相同）
bool sockets::isSelfConnect(int sockfd) {
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if(localaddr.sin6_family == AF_INET){
        const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
        const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port && laddr4->sin_addr.s_addr==raddr4->sin_addr.s_addr;
    }else if(localaddr.sin6_family == AF_INET6){
        return localaddr.sin6_port == peeraddr.sin6_port && memcmp(&localaddr.sin6_addr,&peeraddr.sin6_addr, sizeof(localaddr.sin6_addr) == 0);
    }else{
        return false;
    }
}








