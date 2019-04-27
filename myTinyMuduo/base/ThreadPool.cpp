//
// Created by john on 4/17/19.
//
#include "base/ThreadPool.h"
#include "base/Exception.h"

#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string& nameArg)
    : mutex_(),
      notEmpty_(mutex_),
      notFull_(mutex_),
      name_(nameArg),
      maxQueueSize_(0),
      running_(false)
{}

ThreadPool::~ThreadPool() {
    if(running_){
        stop();
    }
}

void ThreadPool::start(int numThreads) {
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThreads);
    for(int i = 0; i < numThreads; i++){
        char id[32];
        snprintf(id,sizeof(id),"%d", i+1);
        //比push_back快，直接在vector内存区上构造
        //不需要另外移动和复制，Thread对象建在堆上，由unique_ptr管理
        // ThreadPool销毁时Thread也自动销毁
        threads_.emplace_back(new muduo::Thread(std::bind(&ThreadPool::runInThread,this),name_+id));
        threads_[i]->start();
    }
    if(numThreads == 0 && threadInitCallback_){
        threadInitCallback_();
    }
}

void ThreadPool::stop() {
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        //可以让其他线程，都检查到running_变为false了，
        // 其它线程可以退出对条件变量的循环等待了（take()函数中）
        notEmpty_.notifyAll();
    }
    for(auto& thr : threads_){
        thr->join();
    }
}

size_t ThreadPool::queueSize() const {
    MutexLockGuard lock(mutex_);
    return queue_.size();
}

//添加任务
void ThreadPool::run(Task task){
    if(threads_.empty()){
        task();
    }else{
        MutexLockGuard lock(mutex_);
        //等待往队列添加任务
        while(isFull()){
            notFull_.wait();
        }
        assert(!isFull());

        queue_.push_back((std::move(task)));
        //添加任务后，通知其他1个线程，有新任务可以执行
        notEmpty_.notify();
    }
}

ThreadPool::Task ThreadPool::take() {
    MutexLockGuard lock(mutex_);
    //任务队列为空，则放弃互斥锁等待条件达成（有任务放进任务队列）
    //然后再次检查任务队列是否还有任务等待执行（其他线程可能先把任务取出完成了）
    while(queue_.empty() && running_){
        notEmpty_.wait();
    }
    Task task;
    //在确认一次
    if(!queue_.empty()){
        task = queue_.front();
        queue_.pop_front();
        if(maxQueueSize_ > 0){
            //取出任务后，通知生产者可以继续添加任务
            notFull_.notify();
        }
    }
    return task;
}

bool ThreadPool::isFull()const {
    mutex_.assertLocked();
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread() {
    try{
        if(threadInitCallback_){
            threadInitCallback_();
        }
        //不断取任务执行
        while(running_){
            Task task(take());
            if(task){
                task();
            }
        }
    }catch (const Exception& ex){
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }catch(const std::exception& ex){
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }catch (...){
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}













