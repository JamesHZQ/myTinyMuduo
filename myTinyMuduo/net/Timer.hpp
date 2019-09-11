//
// Created by john on 4/19/19.
//

#ifndef MY_TINYMUDUO_NET_TIMER_HPP
#define MY_TINYMUDUO_NET_TIMER_HPP

#include "base/Atomic.hpp"
#include "base/Timestamp.hpp"
#include "net/Callbacks.hpp"

namespace muduo{
    namespace net{
        class Timer:noncopyable{
        public:
            //cb:任务函数，when:执行时间，interval:定时执行任务的间隔
            Timer(TimerCallback cb,Timestamp when, double interval)
                : callback_(std::move(cb)),
                  expiration_(when),
                  interval_(interval),
                  repeat_(interval>0.0),
                  //保证每个定时器拥有唯一ID（线程安全）
                  sequence_(s_numCreated_.incrementAndGet())
            {}

            void run()const{ callback_();}
            Timestamp expiration()const{ return expiration_;}
            bool repeat()const{ return repeat_;}
            int64_t sequence()const{ return sequence_;}
            void restart(Timestamp now);
            static int64_t numCreated(){ return s_numCreated_.get();}

        private:
            const TimerCallback callback_;
            Timestamp           expiration_;
            const double        interval_;
            const bool          repeat_;
            const int64_t       sequence_;

            static AtomicInt64  s_numCreated_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_TIMER_H
