//
// Created by john on 4/27/19.
//

#include "echo.h"

#include "base/Logging.hpp"
#include "net/EventLoop.hpp"

#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

int main()
{
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    InetAddress listenAddr(2007);
    EchoServer server(&loop, listenAddr);
    server.start();
    loop.loop();
}
//use nc localhost 2007 to test
