//
// Created by john on 4/15/19.
//
#include "base/CountDownLatch.hpp"
using namespace muduo;

CountDownLatch::CountDownLatch(int count)
    : mutex_(),
      condition_(mutex_),
      count_(count)
{}

//本线程在计数器减到零以前，一直阻塞
void CountDownLatch::wait() {
    MutexLockGuard lock(mutex_);
    while(count_>0){
        condition_.wait();
    }
}

//递减计数器，在减到零时通知其他所有等待计数器减到零的线程（阻塞在CountDownLatch::wait()的线程）
void CountDownLatch::countDown() {
    MutexLockGuard lock(mutex_);
    --count_;
    if(count_==0){
        condition_.notifyAll();
    }

}

//查看当前计数器的值
int CountDownLatch::getCount() const {
    MutexLockGuard lock(mutex_);
    return count_;
}