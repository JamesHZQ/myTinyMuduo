//
// Created by john on 4/28/19.
//

#include "net/EventLoop.h"
#include "net/TcpServer.h"

using namespace muduo;
using namespace muduo::net;

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime){
    //if (buf->findCRLF()){
        conn->send("No such user\r\n");
        conn->shutdown();
    //}
}

int main()
{
    EventLoop loop;
    TcpServer server(&loop, InetAddress(1079), "Finger");
    server.setMessageCallback(onMessage);
    server.start();
    loop.loop();
}