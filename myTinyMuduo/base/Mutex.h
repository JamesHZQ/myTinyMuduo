//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_MUTEX_H
#define MY_TINYMUDUO_BASE_MUTEX_H

#include "base/CurrentThread.h"
#include "base/noncopyable.h"
#include <assert.h>
#include <pthread.h>


//检查pthread系列函数的返回值，成功返回零
#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret); assert(errnum == 0); (void) errnum;})


namespace muduo{
    class MutexLock:noncopyable{
    public:
        MutexLock():holder_(0){
            //初始化互斥量，不设置互斥量属性
            MCHECK(pthread_mutex_init(&mutex_,NULL));
        }
        ~MutexLock(){
            //在没有线程锁住这个互斥量时，销毁原始互斥量
            assert(holder_==0);
            MCHECK(pthread_mutex_destroy(&mutex_));
        }
        //判断当前线程是否锁住互斥量
        bool isLockedByThisThread()const{
            return holder_==CurrentThread::tid();
        }

        void assertLocked()const{
            assert(isLockedByThisThread());
        }

        //调用线程锁住互斥量，并记录锁住互斥量线程的tid
        void lock(){
            MCHECK(pthread_mutex_lock(&mutex_));
            assignHolder();
        }

        //先解注册，在解锁以免竞争
        void unlock(){
            unassignHolder();
            MCHECK(pthread_mutex_unlock(&mutex_));
        }

        //返回原始互斥量的地址
        pthread_mutex_t* getpthreadMutex(){
            return &mutex_;
        }

    private:
        friend class Condition;

        //RAII：UnassignGuard构造时，解注册；析构时重新注册
        //主要用在Condition::wait，因为其直接使用系统函数解锁/加锁互斥量，没有注册/解注册的步骤
        class UnassignGuard:noncopyable{
        public:
            UnassignGuard(MutexLock& owner):owner_(owner){
                owner_.unassignHolder();
            }
            ~UnassignGuard(){
                owner_.assignHolder();
            }
        private:
            MutexLock &owner_;
        };

        //解注册
        void unassignHolder(){
            holder_=0;
        }
        //注册本线程（记住tid）
        void assignHolder(){
            holder_=CurrentThread::tid();
        }


        pthread_mutex_t mutex_;
        pid_t           holder_;

    };

    //RAII：MutexLockGuard构造时锁住互斥量；析构时解锁互斥量
    class MutexLockGuard:noncopyable{
    public:
        explicit MutexLockGuard(MutexLock& mutex):mutex_(mutex){
            mutex_.lock();
        }
        ~MutexLockGuard(){
            mutex_.unlock();
        }

    private:
        MutexLock& mutex_;
    };
}
#define MutexLockGuard(x) error "Missing guard object name"
#endif //MY_MUDUO_BASE_TEST_MUTEX_H
