//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_CONDITION_HPP
#define MY_TINYMUDUO_BASE_CONDITION_HPP

#include "base/Mutex.hpp"
#include <pthread.h>
#include <errno.h>

namespace muduo{
    class Condition:noncopyable{
    public:
        explicit Condition(MutexLock &mutex):mutex_(mutex){
            //初始化原始条件变量
            int ret = pthread_cond_init(&pcond_,NULL);
            assert(ret==0);
        }
        ~Condition(){
            //销毁原始条件变量
            int ret = pthread_cond_destroy(&pcond_);
            assert(ret==0);
        }
        void wait(){
            //调用pthread_cond_wait后将解锁原始mutex，所以使用UnassignGuard对象解注册，并在重新上锁后再次注册
            MutexLock::UnassignGuard ug(mutex_);
            int ret = pthread_cond_wait(&pcond_,mutex_.getpthreadMutex());
            assert(ret==0);
        }
        bool waitForSeconds(double seconds);
        void notify(){
            //通知某个等待线程（一般表示资源可用）
            int ret = pthread_cond_signal(&pcond_);
            assert(ret==0);
        }
        void notifyAll(){
            //通知所有等待线程（一般表示状态变化）
            int ret = pthread_cond_broadcast(&pcond_);
            assert(ret==0);
        }
    private:
        //MutexLock不可复制
        MutexLock &mutex_;
        pthread_cond_t pcond_;
    };

    //等待超时返回true(纳秒级)
    inline
    bool Condition::waitForSeconds(double seconds) {
        struct timespec abstime;
        //返回时间（纳秒）
        clock_gettime(CLOCK_REALTIME,&abstime);
        const int64_t kNanoSecondsPerSecond = 1000000000;
        //等待时间（纳秒）
        int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

        //计算超时时刻的时间
        abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
        abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

        //pthread_cond_timedwait会释放锁，所以使用UnassignGuard对象解注册，在pthread_cond_timedwait返回时重新上锁
        //随后后UnassignGuard对象析构重新注册。
        MutexLock::UnassignGuard ug(mutex_);
        //超时返回true
        return ETIMEDOUT == pthread_cond_timedwait(&pcond_,mutex_.getpthreadMutex(),&abstime);
    }
}



#endif //MY_MUDUO_BASE_TEST_CONDITION_H
