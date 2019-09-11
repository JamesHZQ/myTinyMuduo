//
// Created by john on 4/19/19.
//

#include "net/TimerQueue.hpp"

#include "base/Logging.hpp"
#include "net/EventLoop.hpp"
#include "net/Timer.hpp"
#include "net/TimerId.hpp"

#include <sys/timerfd.h>
#include <unistd.h>
#include<string.h>

namespace muduo{
    namespace net{
        namespace detail{
            //创建一个timerfd
            int createTimerfd(){
                int timerfd = ::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK|TFD_CLOEXEC);

                if(timerfd < 0){
                    LOG_SYSFATAL<<"Failed in timerfd create";
                }
                return timerfd;
            }
            //计算now到when的微秒数，返回timespec，用于tiemrfd_settime
            struct timespec howMuchTimeFromNow(Timestamp when){
                int64_t microseconds = when.microSecondsSinceEpoch()-Timestamp::now().microSecondsSinceEpoch();
                if(microseconds<100){
                    microseconds = 100;
                }
                struct timespec ts;
                ts.tv_sec = static_cast<time_t >(microseconds/Timestamp::kMicroSecondsPerSecond);
                ts.tv_nsec = static_cast<long>((microseconds%Timestamp::kMicroSecondsPerSecond)*1000);
                return ts;
            }
            //只是用来写日志？
            void readTimerfd(int timerfd,Timestamp now){
                uint64_t howmany;
                ssize_t n = ::read(timerfd,&howmany,sizeof(howmany));
                LOG_TRACE<<"TimerQueue::hanleRead() "<<howmany<<" at "<<now.toString();
                if(n != sizeof(howmany)){
                    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
                }
            }
            //
            void resetTimerfd(int timerfd, Timestamp expiration){
                struct itimerspec newValue;
                struct itimerspec oldValue;
                memset(&newValue, 0, sizeof(newValue));
                memset(&oldValue, 0, sizeof(oldValue));
                newValue.it_value = howMuchTimeFromNow(expiration);
                int ret = ::timerfd_settime(timerfd,0,&newValue,&oldValue);
                if(ret){
                    LOG_SYSERR<<"timer settime()";
                }
            }

        }
    }
}
using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop,timerfd_),
      timers_(),
      callingExpiredTimers_(false)
{
    //设置对应timerfd的Channel的回调函数(传递的是成员函数)
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead,this));
    //开启监听读事件，同时也会更新poll描述符集，添加timerfd_和timerfdChannel_
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    //停止Channel对所有事件的监听
    timerfdChannel_.disableAll();
    //移除Channel
    timerfdChannel_.remove();
    //关闭timerfd文件描述符
    ::close(timerfd_);
    for(const Entry& timer:timers_){
        //删除TimerQueue剩下的Timer对象
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval) {
    Timer* timer = new Timer(std::move(cb), when, interval);
    //添加定时器的任务放到事件循环的任务队列中
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop,this,timer));
    return TimerId(timer,timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer* timer){
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if(earliestChanged){
        //如果加入的定时器是最快过到期的，重新设置timefd
        resetTimerfd(timerfd_,timer->expiration());
    }
}

void TimerQueue::cancel(TimerId timerId) {
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop,this,timerId));
}

void TimerQueue::cancelInLoop(TimerId timerId) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if(it != activeTimers_.end()){
        size_t n = timers_.erase(Entry(it->first->expiration(),it->first));
        assert(n==1); (void)n;
        delete it->first;
        activeTimers_.erase(it);
    }else if(callingExpiredTimers_){
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_,now);

    //获得所有到期的定时器
    std::vector<Entry>expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();

    for(const Entry& it:expired){
        //执行定时器中的任务（事件循环中执行）
        it.second->run();
    }
    callingExpiredTimers_ = false;
    //处理周期定时器
    reset(expired,now);
}
//取出到期的任务
std::vector<TimerQueue::Entry>TimerQueue::getExpired(Timestamp now){
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry>expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    //begin-end之间的定时器的时间戳都比now要小，都是已经过期的
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end()||now<end->first);
    //将到期定时器拷贝到expired（vector）
    std::copy(timers_.begin(),end,back_inserter(expired));
    //从定时器列表中删除已到期期的定时器
    timers_.erase(timers_.begin(),end);

    for(const Entry& it:expired){
        //将到期的定时器从ActiveTimer中删除
        ActiveTimer timer(it.second,it.second->sequence());
        size_t n=activeTimers_.erase(timer);
        assert(n==1);(void)n;
    }

    assert(timers_.size()==activeTimers_.size());
    return expired;

}
void TimerQueue::reset(const std::vector<TimerQueue::Entry> &expired, Timestamp now) {
    Timestamp nextExpire;

    for(const Entry& it:expired){
        ActiveTimer timer(it.second,it.second->sequence());
        if(it.second->repeat()&&cancelingTimers_.find(timer)==cancelingTimers_.end()){
            it.second->restart(now);
            insert(it.second);
        }else{
            delete it.second;
        }
        if(!timers_.empty()){
            nextExpire = timers_.begin()->second->expiration();
        }
        if(nextExpire.valid()){
            resetTimerfd(timerfd_,nextExpire);
        }
    }
}

//往TimerQueue中添加Timer
bool TimerQueue::insert(Timer *timer) {
    loop_->assertInLoopThread();
    assert(timers_.size()==activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it==timers_.end()||when < it->first){
        //TimertQueue为空或插入的定时器是的到期时间是set中最小的，
        // 则最早过期的定时器发生了改变，将标志置为true
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator,bool> result = timers_.insert(Entry(when,timer));
        assert(result.second);(void)result;
    }
    {
        std::pair<ActiveTimerSet::iterator,bool>result = activeTimers_.insert(ActiveTimer(timer,timer->sequence()));
        assert(result.second);(void)result;
    }
    assert(timers_.size()==activeTimers_.size());
    return earliestChanged;
}






























