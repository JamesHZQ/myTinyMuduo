//
// Created by john on 4/28/19.
//
#include "net/EventLoop.h"
#include "net/TcpServer.h"

using namespace muduo;
using namespace muduo::net;

int main(){
    EventLoop loop;
    TcpServer server(&loop,InetAddress(1079),"Figner");
    server.start();
    loop.loop();
}
