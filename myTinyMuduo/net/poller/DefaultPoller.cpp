//
// Created by john on 4/20/19.
//

#include <net/Poller.h>
#include <net/poller/PollPoller.h>
#include <net/poller/EPollPoller.h>

#include <stdlib.h>

using namespace muduo::net;
Poller* Poller::newDefaultPoller(EventLoop *loop) {
    if(::getenv("MUDUO_USE_POLL")){
        return new PollPoller(loop);
    }
}