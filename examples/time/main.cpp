//
// Created by john on 4/29/19.
//

#include "daytime.h"

#include "base/Logging.h"
#include "net/EventLoop.h"

#include <unistd.h>

int main(){
    LOG_INFO<<"PID = "<<getpid();
    EventLoop loop;
    InetAddress listenAddr(2013);
    DaytimeServer server(&loop,listenAddr);
    server.start();
    loop.loop();
}