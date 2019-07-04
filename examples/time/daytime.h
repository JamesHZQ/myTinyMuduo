//
// Created by john on 4/29/19.
//

#ifndef TIME_DAYTIME_H
#define TIME_DAYTIME_H

#include "net/TcpServer.h"

using namespace muduo;
using namespace muduo::net;

class DaytimeServer{
public:
    DaytimeServer(EventLoop* loop,const InetAddress& listenAddr);

    void start();

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);
    TcpServer server_;
};

#endif //TIME_DAYTIME_H
