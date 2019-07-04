//
// Created by john on 4/28/19.
//

#include "net/EventLoop.h"
#include "net/TcpServer.h"

using namespace muduo;
using namespace muduo::net;

typedef std::map<string ,string> UserMap;
UserMap users;
string getUser(const string& user){
    string result = "No such user";
    UserMap::iterator it = users.find(user);
    if(it != users.end()){
        result = it->second;
    }
    return result;
}
void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime){
    //if (buf->findCRLF()){
    string user(buf->retrieveAllAsString());
    conn->send(getUser(user));
    conn->shutdown();
    //}
}

int main()
{
    users[string("john\n")] = "hello\n";
    EventLoop loop;
    TcpServer server(&loop, InetAddress(1079), "Finger");
    server.setMessageCallback(onMessage);
    server.start();
    loop.loop();
}
