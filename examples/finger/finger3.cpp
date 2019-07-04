//
// Created by john on 4/28/19.
//

#include "net/EventLoop.h"
#include "net/TcpServer.h"

using namespace muduo;
using namespace muduo::net;

void onConnection(const TcpConnectionPtr& conn){
    if(conn->connected()){
        conn->shutdown();
    }
}
int main(){
    EventLoop loop;
    TcpServer server(&loop,InetAddress(1079),"Finger");
    server.setConnectionCallback(onConnection);
    server.start();
    loop.loop();
}