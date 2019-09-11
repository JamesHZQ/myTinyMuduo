//
// Created by john on 4/15/19.
//
#include "base/Condition.hpp"

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

    //pthread_cond_timedwait会释放锁，所以使用UnassignGuard对象解注册，在pthread_cond_timedwait返回时重新上锁
    //随后后UnassignGuard对象析构重新注册。
    MutexLock::UnassignGuard ug(mutex_);
    //超时返回true
    return ETIMEDOUT == pthread_cond_timedwait(&pcond_,mutex_.getpthreadMutex(),&abstime);
}
