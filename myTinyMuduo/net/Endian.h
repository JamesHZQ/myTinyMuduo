//
// Created by john on 4/18/19.
//

#ifndef MY_TINYMUDUO_NET_ENDIAN_H
#define MY_TINYMUDUO_NET_ENDIAN_H

#include <stdint.h>
#include <endian.h>
namespace muduo{
    namespace net{
        namespace sockets{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
            //主机序转换为网络序
            inline uint64_t hostToNetwork64(uint64_t host64){
                return htobe64(host64);
            }
            inline uint32_t hostToNetwork32(uint32_t host32){
                return htobe32(host32);
            }
            inline uint16_t hostToNetwork16(uint16_t host16){
                return htobe16(host16);
            }

            //网络序转换为主机序
            inline uint64_t networkToHost64(uint64_t net64){
                return be64toh(net64);
            }
            inline uint32_t networkTohost32(uint32_t net32){
                return be32toh(net32);
            }
            inline uint16_t networkToHost16(uint16_t net16){
                return be16toh(net16);
            }
#pragma GCC diagnostic pop
        }
    }
}

#endif //MY_TINYMUDUO_NET_ENDIAN_H
