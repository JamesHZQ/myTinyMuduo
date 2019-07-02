//
// Created by john on 4/21/19.
//

#include "net/InetAddress.h"

#include "base/Logging.h"
#include "net/Endian.h"
#include "net/SocketsOps.h"

#include <netdb.h>
#include <netinet/in.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
//本机任意可绑定IP地址
static const in_addr_t kInaddrAny = INADDR_ANY;
//环回地址
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. （二进制IP地址）*/
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };
using namespace muduo;
using namespace muduo::net;

//编译期查看InetAddress对象包含的是IPV4地址还是IPV6地址
static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6), "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

//仅指定端口，是否环回（否则取任意地址），是否IPV6
InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6) {
    static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
    static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");

    if(ipv6){
        memZero(&addr6_, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly?in6addr_loopback:in6addr_any;
        //IPV6地址不需要转换为网络字节序（in6addr_loopback和in6addr_any已经是网络字节序）
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = sockets::hostToNetwork16(port);
    }else{
        memZero(&addr_, sizeof(addr_));
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback:kInaddrAny;
        //将端口和IP地址转换为网络字节序
        addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip);
        addr_.sin_port = sockets::hostToNetwork16(port);
    }
}

//制定IP地址，端口，是否IPV6
InetAddress::InetAddress(StringArg ip, uint16_t port, bool ipv6) {
    if(ipv6){
        memZero(&addr6_, sizeof(addr6_));
        //将ip转换为c字符串，调用fromIpPort设置addr6_
        sockets::fromIpPort(ip.c_str(),port,&addr6_);
    }else{
        memZero(&addr_, sizeof(addr_));
        sockets::fromIpPort(ip.c_str(),port,&addr_);
    }
}

string InetAddress::toIpPort() const {
    char buf[64] = "";
    sockets::toIpPort(buf, sizeof(buf),getSockAddr());
    return buf;
}
uint32_t InetAddress::ipNetEndian() const {
    assert(family() == AF_INET);
    //返回IPV4地址（二进制整数）
    return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::toPort() const {
    return sockets::networkToHost16(portNetEndian());
}

static __thread char t_resolveBuffer[64*1024];
//DNS解析
bool InetAddress::resolve(StringArg hostname, InetAddress *out) {
    assert(out != NULL);
    struct hostent hent;
    struct hostent* he = NULL;
    int herrno = 0;
    memZero(&hent, sizeof(hent));
    int ret = gethostbyname_r(hostname.c_str(),&hent,t_resolveBuffer, sizeof(t_resolveBuffer), &he, &herrno);
    if(ret == 0 && he != NULL){
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        return true;
    }else{
        if(ret){
            LOG_SYSERR<<"InetAddress::resolve";
        }
        return false;
    }
}

void InetAddress::setScopeID(uint32_t scope_id) {
    if(family()==AF_INET6){
        addr6_.sin6_scope_id = scope_id;
    }
}









