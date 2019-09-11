//
// Created by john on 4/19/19.
//

#ifndef MY_TINYMUDUO_NET_TIMERQUEUE_HPP
#define MY_TINYMUDUO_NET_TIMERQUEUE_HPP

#include "base/Mutex.hpp"
#include "base/Timestamp.hpp"
#include "net/Callbacks.hpp"
#include "net/Channel.hpp"

#include <set>
#include <vector>

namespace muduo{
    namespace net{
        class EventLoop;
        class Timer;
        class TimerId;
        class TimerQueue:noncopyable{
        public:
            explicit TimerQueue(EventLoop* loop);
            ~TimerQueue();

            TimerId addTimer(TimerCallback cb, Timestamp when,double interval);
            void cancel(TimerId timerId);
        private:
            //为了让每个Timer对应的map关键字不相同，
            //不能仅以到期时间为关键字，还需要加入Timer对象的地址组成pair对象
            //使用set的原因是它是一种二叉树结构，查找的事件复杂度为log(n)
            //可以快速找到已到期的Timer对象
            typedef std::pair<Timestamp, Timer*> Entry;
            typedef std::set<Entry> TimerList;
            typedef std::pair<Timer*, int64_t> ActiveTimer;
            typedef std::set<ActiveTimer> ActiveTimerSet;

            void addTimerInLoop(Timer* timer);
            void cancelInLoop(TimerId timerId);

            void handleRead();

            std::vector<Entry> getExpired(Timestamp now);
            void reset(const std::vector<Entry>& expired, Timestamp now);

            bool insert(Timer* timer);

            EventLoop*      loop_;                  //持有该Timerueue的EventLoop
            const int       timerfd_;               //内部的timerfd
            Channel         timerfdChannel_;        //timerfd对应的Channel
            TimerList       timers_;                //所有定时任务

            ActiveTimerSet  activeTimers_;
            bool            callingExpiredTimers_;
            ActiveTimerSet  cancelingTimers_;

        };
    }
}

#endif //MY_TINYMUDUO_NET_TIMERQUEUE_H
