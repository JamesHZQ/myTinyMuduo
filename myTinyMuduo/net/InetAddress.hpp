//
// Created by john on 4/19/19.
//

#ifndef MY_TINYMUDUO_NET_INETADDRESS_HPP
#define MY_TINYMUDUO_NET_INETADDRESS_HPP

#include "base/copyable.hpp"
#include "base/StringPiece.hpp"

#include <netinet/in.h>
#include <string>
namespace muduo{
    namespace net{
        namespace sockets{
            const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
        }
        //简单封装sockaddr_in
        class InetAddress:public muduo::copyable{
        public:

            //给定端口号构造节点，主要用在TcpServer listening
            explicit InetAddress(uint16_t port = 0,bool loopbackOnly = false, bool ipv6 = false);
            //给定IP和端口构造节点“1.2.3.4：5678”
            InetAddress(StringArg ip,uint16_t port, bool ipv6 = false);
            //给定sockaddr_in构造节点
            explicit InetAddress(const struct sockaddr_in& addr)
                    : addr_(addr)
            {}
            explicit InetAddress(const struct sockaddr_in6& addr)
                    : addr6_(addr)
            {}

            sa_family_t family()const{return addr_.sin_family;}
            std::string toIp()const;
            std::string toIpPort()const;
            uint16_t toPort()const;

            const struct sockaddr* getSockAddr()const{return sockets::sockaddr_cast(&addr6_);}
            void setSockAddrInet6(const struct sockaddr_in6& addr6){addr6_ = addr6;}

            uint32_t ipNetEndian()const;
            uint16_t portNetEndian()const {return addr_.sin_port;}

            static bool resolve(StringArg hostname, InetAddress* out);

            void setScopeID(uint32_t scope_id);
        private:
            union{
                struct sockaddr_in addr_;
                struct sockaddr_in6 addr6_;
            };
        };
    }
}

#endif //MY_TINYMUDUO_NET_INETADDRESS_H
