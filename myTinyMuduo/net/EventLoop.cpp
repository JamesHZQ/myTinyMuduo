//
// Created by john on 4/18/19.
//
#include "net/EventLoop.h"
#include "base/Logging.h"
#include "base/Mutex.h"
#include "net/Channel.h"
#include "net/Poller.h"
#include "net/SocketsOps.h"
#include "net/TimerQueue.h"

#include <algorithm>

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

namespace {
    //线程特定数据
    //这个变量保存线程持有的EvenLoop对象的地址（一个线程至多持有一个EventLoop对象）
    __thread EventLoop* t_loopInThisThread = 0;

    const int kPollTimeMs = 10000;

    int createEventfd(){
        int evtfd = ::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
        if(evtfd<0){
            LOG_SYSERR<<"Failed in eventfd";
            abort();
        }
        return evtfd;
    }

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        //屏蔽SIGPIPE信号（对于一条关闭的TCP连接，第一次发数据收到RST报文，第二次收到SIGPIPE）
        ::signal(SIGPIPE, SIG_IGN);
        // LOG_TRACE << "Ignore SIGPIPE";
    }
};
#pragma GCC diagnostic error "-Wold-style-cast"

//全局对象在main函数开始前，通过IgnoreSigPipe对象，屏蔽了SIGPIPE信号
IgnoreSigPipe initObj;

}  // namespace

//这是一个类静态函数，不属于某个类的对象，若线程中创建了EventLoop对象，该函数返回其地址，
//若没有创建EventLoop对象，该函数返回NULL。该函数可用于判断当前线程是否是IO线程（有没有EventLoop对象）
EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      iteration_(0),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this,wakeupFd_)),
      currentActiveChannel_(NULL)
{
    LOG_DEBUG<<"EventLoop created "<< this <<" in thread "<<threadId_;
    //线程特定数据t_loopInThisThread不为0，
    //表示已经有其它EventLoop对象在这个线程里初始化，而一个线程至多只能有一个事件循环
    //所以t_loopInThisThread不为0时中止程序，否则初始化t_loopInThisThread，
    //为当前EventLoop对象的地址
    if(t_loopInThisThread){
        LOG_SYSFATAL<<"Another EventLoop "<<t_loopInThisThread<<" exists in this thread "<<threadId_;
    }else{
        t_loopInThisThread = this;
    }
    //设置wakeupChannel_回调函数，在往wakeupFd_写入数据后，触发wakeupFd_的读事件
    //wakeupChannel_会调用读事件处理函数
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //打开对读事件监听的同时，也会更新poll描述符集，添加wakeupFd_及wakeupChannel_
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    LOG_DEBUG<<"EventLoop "<<this<<" of thread "<<threadId_
            <<" destructs in thread "<< CurrentThread::tid();
    //清理wakeup相关的资源
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;      //本IO线程变为普通线程
}
void EventLoop::loop(){
    assert(!looping_);
    //只能在EventLoop所属线程调用
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE<<"EventLoop "<<this<<" start looping";

    while(!quit_){
        //每次调用poll()就要重新填充新的有事件发生的channel，所以每次都要清空
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
        ++iteration_;
        if(Logger::logLevel()<=Logger::TRACE){
            printActiveChannels();
        }
        //开始处理事件
        eventHandling_ = true;
        for(Channel* channel:activeChannels_){
            currentActiveChannel_ = channel;
            //对每个有事件发生的Channel进行事件处理
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_=NULL;
        eventHandling_ = false;
        //处理任务队列中的任务
        doPendingFunctors();
    }
    LOG_TRACE<<"EventLoop "<<this<<" stop looping";
    looping_ = false;
}
void EventLoop::quit() {
    //在本EventLoop所属线程调用时，不必wakeup（quit_原子量），
    //本EventLoop所属线程调用quit()说明没有阻塞在poll()
    quit_ = true;
    //其他线程调用本线程的EventLoop对象的quit()时，
    // 要用wakeup让本线程不再阻塞在poll调用，可以更加迅速的响应退出事件循环
    if(!isInloopThread()){
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if(isInloopThread()){
        //如果是本EventLoop所属线程调用（一般是事件处理，任务处理），直接调用Functor
        cb();
    }else{
        //如果是其它的线程调用，为避免竞争将任务传给EventLoop所属对象，等待执行
        queueInLoop(std::move(cb));
    }
}


void EventLoop::queueInLoop(Functor cb) {
    {
        //可能有多个线程调用queueInLoop往同一个线程中的EventLoop的任务队列里添加任务，
        // 所以要加锁保护pendingFunctors_
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    //其它线程为了能尽快让任务执行，需要调用本EventLoop的wakeup，让本EventLoop不会阻塞在poll调用
    //另外，如果本线程调用queueInLoop，本EventLoop正在执行，此时添加的任务是不会在执行的，需要等下个循环
    //同样面临阻塞在poll的问题，所以也要调用wakeup
    if(!isInloopThread()||callingPendingFunctors_){
        wakeup();
    }
}

size_t EventLoop::queueSize() const {
    MutexLockGuard lock(mutex_);
    return pendingFunctors_.size();
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb) {
    return timerQueue_->addTimer(std::move(cb),time,0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb) {
    Timestamp time(addTime(Timestamp::now(),delay));
    return runAt(time,std::move(cb));
}

//重复执行多次
TimerId EventLoop::runEvery(double interval, TimerCallback cb) {
    Timestamp time(addTime(Timestamp::now(),interval));
    return timerQueue_->addTimer(std::move(cb),time,interval);
}

void EventLoop::cancel(TimerId timerId) {
    return timerQueue_->cancel(timerId);
}

//更新poll里的描述符集
void EventLoop::updateChannel(Channel *channel) {
    assert(channel->owerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}


void EventLoop::removeChannel(Channel *channel) {
    assert(channel->owerLoop() == this);
    assertInLoopThread();
    //要删除的channel如果在activeChannels_里，只能删除正在处理的currentActiveChannel_
    if(eventHandling_){
        assert(currentActiveChannel_ == channel ||
        std::find(activeChannels_.begin(),activeChannels_.end(),channel)==activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    assert(channel->owerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

void EventLoop::abortNotInloopThread() {
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " <<  CurrentThread::tid();
}
//激活poll或epoll
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n=sockets::write(wakeupFd_,&one, sizeof(one));
    if(n != sizeof(one)){
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n=sockets::read(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one)){
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        //取出任务队列现有的所有任务
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    //执行刚才取出的任务
    for(const Functor& functor:functors){
        functor();
    }
    callingPendingFunctors_ = false;
}
void EventLoop::printActiveChannels() const {
    for(const Channel* channel : activeChannels_){
        LOG_TRACE<<"{"<<channel->reventsToString()<<"} ";
    }
}













