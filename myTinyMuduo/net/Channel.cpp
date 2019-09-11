//
// Created by john on 4/18/19.
//
#include "base/Logging.hpp"
#include "net/Channel.hpp"
#include "net/EventLoop.hpp"

#include <sstream>
#include <poll.h>
#include <string>
using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;                  //没有事件
const int Channel::kReadEvent = POLLIN | POLLPRI;   //读事件
const int Channel::kWriteEvent = POLLOUT;           //写事件

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      logHup_(true),
      tied_(false),
      eventHandling_(false),
      addedToloop_(false)
{}
Channel::~Channel() {

    assert(!eventHandling_);
    assert(!addedToloop_);
    if(loop_->isInloopThread()){
        //确保EventLoop和Poller中移除了该Channel
        assert(!loop_->hasChannel(this));
    }
}
void Channel::tie(const std::shared_ptr<void> & obj) {
    //使用weak_ptr弱绑定obj，可以探测obj的存活或者延长obj的生命期
    tie_ = obj;
    tied_ = true;
}

//通过调用EventLoop中的updateChannel改变Poller中的描述符集（vector<struct pollfd>对象和ChannelMap对象）
void Channel::update() {
    addedToloop_ = true;
    loop_->updateChannel(this);
}

//通过调用EventLoop中的removeChannel，移除Poller的描述符集中的本Channel对象
void Channel::remove() {
    assert(isNoneEvent());
    addedToloop_ = false;
    loop_->removeChannel(this);
}

//处理事件
void Channel::handleEvent(Timestamp receiveTime) {
    std::shared_ptr<void> guard;
    if(tied_){
        //若weak_ptr弱绑定的对象此刻存活，将其提升为shared_ptr强绑定
        //确保在handleEvent中，被绑定的对象不会被销毁
        guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }
    else{
        handleEventWithGuard(receiveTime);
    }
}
//处理事件
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    eventHandling_ = true;
    LOG_TRACE<<reventsToString();
    //根据发生的事件类型执行不同的回调函数
    if((revents_&POLLHUP)&&!(revents_&POLLIN)){
        if(logHup_){
            LOG_WARN<<"fd = "<<fd_<<" Channel::handle_event() POLLHUP";
        }
        if(closeCallback_) closeCallback_();
    }
    if(revents_ & POLLNVAL){
        LOG_WARN<<"fd = "<<fd_<<" Channel::handle_event() POLLNVAL";
    }
    if(revents_ & (POLLERR|POLLNVAL)){
        if(errorCallback_) errorCallback_();
    }
    if(revents_ & (POLLIN|POLLPRI|POLLRDHUP)){
        if(readCallback_) readCallback_(receiveTime);
    }
    if(revents_ & POLLOUT){
        if(writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
}

std::string Channel::reventsToString() const {
    return eventsToString(fd_,revents_);
}
std::string Channel::eventsToString() const {
    return eventsToString(fd_,events_);
}
std::string Channel::eventsToString(int fd,int ev) {
    std::ostringstream oss;
    oss<<fd<<": ";
    if(ev & POLLIN)
        oss<<"IN ";
    if(ev & POLLPRI)
        oss<<"PRI";
    if(ev&POLLOUT)
        oss<<"OUT";
    if(ev&POLLHUP)
        oss<<"HUP";
    if(ev&POLLRDHUP)
        oss<<"REDHUP";
    if(ev&POLLERR)
        oss<<"ERR";
    if(ev&POLLNVAL)
        oss<<"NVAL";
    return oss.str();
}
