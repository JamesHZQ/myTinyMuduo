#include "daytime.h"

#include "base/Logging.h"
#include "net/EventLoop.h"

DaytimeServer::DaytimeServer(EventLoop *loop, const InetAddress &listenAddr)
    : server_(loop,listenAddr,"DaytimeServer")
{
    server_.setConnectionCallback(std::bind(&DaytimeServer::onConnection,this,_1));
    server_.setMessageCallback(std::bind(&DaytimeServer::onMessage,this,_1,_2,_3));
}
void DaytimeServer::start() {
    server_.start();
}
void DaytimeServer::onConnection(const TcpConnectionPtr &conn) {
    LOG_INFO<<"daytimeServer - "<<conn->peerAddress().toIpPort()<<" -> "
            <<conn->localAddress().toIpPort()<<" is "
            <<(conn->connected()?"up":"Down");
    if(conn->connected()){
        //conn->send(Timestamp::now().toFormattedString()+"\n");
        time_t now = time(NULL);
        int32_t be32 = sockets::hostToNetwork32(static_cast<int32_t >(now));
        conn->send(&be32, sizeof(be32));
        conn->shutdown();
    }
}

void DaytimeServer::onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time){
    string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name()<<" discards "<<msg.size()
             << " bytes received at "<< time.toString();
}