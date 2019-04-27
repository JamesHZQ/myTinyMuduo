//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_COUNTDOWNLATCH_H
#define MY_TINYMUDUO_BASE_COUNTDOWNLATCH_H

#include "base/Condition.h"
#include "base/Mutex.h"

//带锁的计数器
namespace muduo{
    class CountDownLatch:noncopyable{
    public:
        explicit CountDownLatch(int count);
        void wait();
        void countDown();
        int getCount() const;

    private:
        mutable MutexLock mutex_;
        Condition condition_;
        int count_;
    };
}

#endif //MY_MUDUO_BASE_TEST_COUNTDOWNLATCH_H
