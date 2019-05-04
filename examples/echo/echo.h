//
// Created by john on 4/27/19.
//

#ifndef ECHO_ECHO_H
#define ECHO_ECHO_H

#include "net/TcpServer.h"

class EchoServer{
public:
    EchoServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr);

    void start();

private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn);

    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                    muduo::net::Buffer*buffer,
                    muduo::Timestamp time);

    muduo::net::TcpServer server_;
};

#endif //ECHO_ECHO_H
