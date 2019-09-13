//
// Created by john on 4/27/19.
//

#include "echo.h"

#include "base/Logging.hpp"

#include<string>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using namespace muduo;
using namespace muduo::net;
using namespace std;

EchoServer::EchoServer(EventLoop *loop,
                       const InetAddress &listenAddr)
    : server_(loop,listenAddr,"EchoServer")
{
    server_.setConnectionCallback(std::bind(&EchoServer::onConnection,this,_1));
    server_.setMessageCallback(std::bind(&EchoServer::onMessage,this,_1,_2,_3));
}

void EchoServer::start() {
    server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr &conn) {
    LOG_INFO<<"EchoServer - "<<conn->peerAddress().toIpPort()<<"->"
                << conn->localAddress().toIpPort() << " is "
                << (conn->connected() ? "UP" : "DOWN");
}

void EchoServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
             << "data received at " << time.toString();
    conn->send(msg);
}
