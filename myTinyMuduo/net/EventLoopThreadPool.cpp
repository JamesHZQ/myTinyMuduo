//
// Created by john on 4/25/19.
//

#include "net/EventLoopThreadPool.hpp"

#include "net/EventLoop.hpp"
#include "net/EventLoopThread.hpp"

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0)
{}

EventLoopThreadPool::~EventLoopThreadPool() {

}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    assert(!started_);
    //不允许跨线程，只能在baseLoop_所在的主线程里创建线程池
    baseLoop_->assertInLoopThread();

    started_ = true;

    for(int i=0;i<numThreads_;++i){
        char buf[name_.size()+32];
        snprintf(buf, sizeof(buf),"%s%d",name_.c_str(),i);
        //每个EventLoopThread(在堆上，通过unique_ptr管理)对象，
        //会创建一个新线程，并在新线程的栈上创建EventLoop对象，
        EventLoopThread* t = new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        //开启事件循环，并得到EventLoop对象的指针
        loops_.push_back(t->startLoop());
    }
    if(numThreads_ == 0 && cb){
        cb(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    //只有主线程进行，线程间的调度
    baseLoop_->assertInLoopThread();
    assert(started_);

    //没有创建其他线程的时候返回主线程的EventLoop对象的指针
    EventLoop* loop = baseLoop_;
    //每个线程轮流接受任务
    if(!loops_.empty()){
        loop = loops_[next_];
        ++next_;
        if(static_cast<size_t >(next_) >= loops_.size()){
            next_=0;
        }
    }
    return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode) {
    baseLoop_->assertInLoopThread();
    EventLoop* loop = baseLoop_;

    if(!loops_.empty()){
        //根据输入的hashCode决定由哪个线程来接受任务
        loop = loops_[hashCode % loops_.size()];
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    baseLoop_->assertInLoopThread();
    assert(started_);
    if(loops_.empty()){
        return std::vector<EventLoop*>(1,baseLoop_);
    }else{
        return loops_;
    }
}














