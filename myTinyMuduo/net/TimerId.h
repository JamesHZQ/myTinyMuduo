//
// Created by john on 4/17/19.
//

#ifndef MY_TINYMUDUO_NET_TIMERID_H
#define MY_TINYMUDUO_NET_TIMERID_H

#include "base/copyable.h"

namespace muduo{
    namespace net{
        class Timer;

    class TimerId:public muduo::copyable{
    public:
        TimerId()
            : timer_(NULL),
              sequence_(0)
        {}
        TimerId(Timer* timer, int64_t seq)
            : timer_(timer),
              sequence_(seq)
        {}
        friend class TimerQueue;

    private:
        Timer*   timer_;
        int64_t  sequence_;
    };
    }
}
#endif //MY_TINYMUDUO_NET_TIMERID_H
