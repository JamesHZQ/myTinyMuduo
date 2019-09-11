//
// Created by john on 4/20/19.
//

#include "net/Poller.hpp"
#include "net/poller/PollPoller.hpp"
#include "net/poller/EPollPoller.hpp"

#include <stdlib.h>

using namespace muduo::net;
Poller* Poller::newDefaultPoller(EventLoop *loop) {
//    if (::getenv("MUDUO_USE_POLL"))
//    {
//        return new PollPoller(loop);
//    }
//    else
//    {
//        return new EPollPoller(loop);
//    }
    return new EPollPoller(loop);
}