//
// Created by john on 5/4/19.
//

#ifndef CHAT_CODEC_H
#define CHAT_CODEC_H

#include "base/Logging.h"
#include "net/Buffer.h"
#include "net/Endian.h"
#include "net/TcpServer.h"

class LengthHeaderCodec : muduo::noncopyable{
public:
    typedef std::function<void (const muduo::net::TcpConnectionPtr&,
                                const muduo::string& message,
                                muduo::Timestamp)> StringMessageCallback;
    explicit LengthHeaderCodec(const StringMessageCallback& cb)
        : messageCallback_(cb)
    {}

    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                         muduo::net::Buffer* buf,
                         muduo::Timestamp receiveTime)
    {
        //buf缓冲区可能有包含数个消息
        while(buf->readableBytes()>=kHeaderLen){
            const void* data = buf->peek();
            int32_t be32 = *static_cast<const int32_t*>(data);
            //确定消息长度
            const int32_t len = muduo::net::sockets::networkTohost32(be32);
            if(len > 65535||len<0){
                LOG_ERROR<<"Invalid length "<<len;
                conn->shutdown();
                break;
                //buf包含一个完整的消息
            }else if(buf->readableBytes() >= len + kHeaderLen){
                buf->retrieve(kHeaderLen);
                muduo::string message(buf->peek(),len);
                messageCallback_(conn,message,receiveTime);
                buf->retrieve(len);
                //若消息没有接收完，退出循环等待接受剩余的数据
            }else{
                break;
            }
        }
    }

    void send(muduo::net::TcpConnection* conn,const muduo::StringPiece& message){
        muduo::net::Buffer buf;
        buf.append(message.data(),message.size());
        int32_t len = static_cast<int32_t >(message.size());
        int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
        //将消息长度写到buffer的prepend区
        buf.prepend(&be32,sizeof(be32));
        //将整个buffer作为消息发送出去
        conn->send(&buf);
    }
private:
    StringMessageCallback messageCallback_;
    const static size_t kHeaderLen = sizeof(int32_t);
};


#endif //CHAT_CODEC_H
