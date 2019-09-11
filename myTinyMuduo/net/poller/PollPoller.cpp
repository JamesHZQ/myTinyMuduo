//
// Created by john on 4/19/19.
//

#include "net/poller/PollPoller.hpp"

#include "base/Logging.hpp"
#include "net/Channel.hpp"

#include <assert.h>
#include <errno.h>
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

PollPoller::PollPoller(EventLoop *loop)
    : Poller(loop)
{}

PollPoller::~PollPoller() =default;

Timestamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    //调用poll()，对pollfds_中发生了感兴趣事件（events）的，设置相应的revents，
    // numEvents发生的事件数，超时时间为timeoutMs微秒，一直没有事件发生直到超时，
    //poll系统调用返回零
    int numEvents = ::poll(&*pollfds_.begin(),pollfds_.size(),timeoutMs);
    int savedErrno = errno;
    //返回poll调用返回的时间
    Timestamp now(Timestamp::now());
    if(numEvents > 0){
        LOG_TRACE<<numEvents<<" events happened";
        //根据poll的返回结果，添加相应的Channel
        fillActiveChannels(numEvents,activeChannels);
    }else if(numEvents == 0){
        LOG_TRACE<<" nothing happened";
    }else{
        if(savedErrno != EINTR){
            errno = savedErrno;
            LOG_SYSERR<<" PollPoller::poll()";
        }
    }
    //now为poll调用返回的时刻
    return now;
}

//根据poll
void PollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    //遍历pollfds_（找出所有活跃套接字后退出循环）
    //找到numEvents个就可以退出了
    for(PollFdList::const_iterator pfd = pollfds_.begin();
        pfd != pollfds_.end()&&numEvents>0;++pfd){
        //revents>0表示有感兴趣的事件发生了
        if(pfd->revents>0){
            --numEvents;
            //在ChannelMap里找到对应fd的Channle对象
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel* channel = ch->second;
            assert(channel->fd() == pfd->fd);
            //设置Channel的发生了的事件类型
            channel->set_revents(pfd->revents);
            activeChannels->push_back(channel);
        }
    }
}

void PollPoller::updateChannel(Channel *channel) {
    //不能跨线程
    Poller::asserInLoopThread();
    LOG_TRACE<<"fd = "<<channel->fd()<<" events = "<<channel->events();
    //channel还没放入ChannelMap和pollfds（vector<struct pollfd>）_
    if(channel->index() < 0){
        //将一个新的channel(fd)加入事件循环
        //将channel添加到pollfds_
        //更新channel的index_，建立channel到pollfds_的映射
        //更新ChannelMap，建立fd到channel的映射
        assert(channels_.find(channel->fd())==channels_.end());
        struct pollfd pfd;
        //初始化poolfd
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);

        int idx = static_cast<int>(pollfds_.size())-1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    } else{
        //channel已存在于ChannelMap和pollfds（vector<struct pollfd>）
        //主要用来更新pollfds_中的fd和events
        assert(channels_.find(channel->fd())!=channels_.end());
        assert(channels_[channel->fd()]==channel);
        int idx = channel->index();
        assert(0<=idx&&idx<static_cast<int>(pollfds_.size()));
        //通过channel的index，在pollfds可以找到该channel对应的fd
        struct pollfd &pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd()||pfd.fd==-channel->fd()-1);
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        //如果channel没有感兴趣的对象
        if(channel->isNoneEvent()){
            //忽略该pollfd
            pfd.fd = -channel->fd()-1;
        }
    }
}

void PollPoller::removeChannel(Channel *channel) {
    Poller::asserInLoopThread();
    //以下主要是用来断定channel已注册并且没有要监听的事件
    LOG_TRACE<<"fd = "<<channel->fd();
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());
    int idx = channel->index();
    assert(0<=idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd& pfd = pollfds_[idx];(void)pfd;
    assert(pfd.fd == -channel->fd()-1&&pfd.events == channel->events());
    //删除ChannelMap中相应的channel
    size_t n = channels_.erase(channel->fd());
    assert(n==1);(void)n;
    //要是最后一个直接删除
    if(static_cast<size_t>(idx) == pollfds_.size()-1){
        pollfds_.pop_back();
    }else{
        int channelAtEnd = pollfds_.back().fd;
        //将要删除的pfd与pollfds_的最后一个交换，然后通过pop()删除，这样比较快
        iter_swap(pollfds_.begin()+idx,pollfds_.end()-1);
        //更新原来pollfds_的最后一个pfd对应的channel的index
        if(channelAtEnd<0){
            //若fd为负，将其还原，通过ChannelMap找到对应的channel
            channelAtEnd = -channelAtEnd-1;
        }
        //更新pollfds_原本最后一个pfd对应的channel的idx
        channels_[channelAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}