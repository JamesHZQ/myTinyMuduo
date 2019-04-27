//
// Created by john on 4/15/19.
//
#include "base/Condition.h"

#include <errno.h>

//等待超时返回true(seconds可以是小数)
bool muduo::Condition::waitForSeconds(double seconds) {

    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME,&abstime);
    const int64_t kNanoSecondsPerSecond = 1000000000;
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

    //计算后abstime为超时时刻的时间
    abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
    abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

    MutexLock::UnassignGuard ug(mutex_);
    return ETIMEDOUT == pthread_cond_timedwait(&pcond_,mutex_.getpthreadMutex(),&abstime);
}
