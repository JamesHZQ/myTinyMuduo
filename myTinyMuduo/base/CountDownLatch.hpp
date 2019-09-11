//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_COUNTDOWNLATCH_HPP
#define MY_TINYMUDUO_BASE_COUNTDOWNLATCH_HPP

#include "base/Condition.hpp"
#include "base/Mutex.hpp"

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
