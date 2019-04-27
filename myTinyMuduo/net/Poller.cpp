//
// Created by john on 4/19/19.
//
#include "net/Poller.h"
#include "net/Channel.h"

using namespace muduo;
using namespace muduo::net;

Poller::Poller(muduo::net::EventLoop *loop)
    : ownerLoop_(loop)
{}

Poller::~Poller() = default;

bool Poller::hasChannel(muduo::net::Channel *channel) const {
    asserInLoopThread();
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}