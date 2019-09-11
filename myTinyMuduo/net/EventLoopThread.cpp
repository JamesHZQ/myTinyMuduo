//
// Created by john on 4/24/19.
//

#include "net/EventLoopThread.hpp"
#include "net/EventLoop.hpp"

using namespace muduo;
using namespace muduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(NULL),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc,this),name),
      mutex_(),
      cond_(mutex_),
      callback_(cb)
{}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    //若loop_==NULL说明开启的线程已经结束了（不用管了）
    if(loop_!=NULL){
        //EventLoop对象中止事件循环
        loop_->quit();
        //等到开启的线程结束
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop() {
    assert(!thread_.started());
    thread_.start();
    EventLoop* loop = NULL;
    {
        MutexLockGuard lock(mutex_);
        while(loop_ == NULL){
            cond_.wait();
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc() {
    //EvevntLoop对象（loop）是新开启的线程的栈变量
    //那个线程结束后EventLoop对象自动销毁
    EventLoop loop;
    if(callback_){
        callback_(&loop);
    }
    //通知主线程EventLoop对象已经创建好了
    //主线程收到通知后立即锁住mutex_,记录EventLoop对象的地址
    //这样是为了避免loop.loop()在很短的时间就退出了
    //那个时候新线程的EventLoop栈对象，已经销毁
    //
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }
    loop.loop();
    //等待startLoop记录EventLoop对象的地址
    MutexLockGuard lock(mutex_);
    loop_ = NULL;
}
