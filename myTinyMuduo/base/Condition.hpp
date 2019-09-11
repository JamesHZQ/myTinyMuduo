//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_CONDITION_HPP
#define MY_TINYMUDUO_BASE_CONDITION_HPP

#include "base/Mutex.hpp"
#include <pthread.h>

namespace muduo{
    class Condition:noncopyable{
    public:
        explicit Condition(MutexLock &mutex):mutex_(mutex){
            //初始化原始条件变量
            MCHECK(pthread_cond_init(&pcond_,NULL));
        }

        ~Condition(){
            //销毁原始条件变量
            MCHECK(pthread_cond_destroy(&pcond_));
        }
        void wait(){
            //调用pthread_cond_wait后将解锁原始mutex，所以使用UnassignGuard对象解注册，并在重新上锁后再次注册
            MutexLock::UnassignGuard ug(mutex_);
            MCHECK(pthread_cond_wait(&pcond_,mutex_.getpthreadMutex()));
        }
        bool waitForSeconds(double seconds);
        void notify(){
            //通知某个等待线程（一般表示资源可用）
            MCHECK(pthread_cond_signal(&pcond_));
        }
        void notifyAll(){
            //通知所有等待线程（一般表示状态变化）
            MCHECK(pthread_cond_broadcast(&pcond_));
        }
    private:
        //MutexLock不可复制
        MutexLock &mutex_;
        pthread_cond_t pcond_;
    };
}



#endif //MY_MUDUO_BASE_TEST_CONDITION_H
