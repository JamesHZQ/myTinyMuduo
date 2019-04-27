//
// Created by john on 4/17/19.
//

#ifndef MY_TINYMUDUO_BASE_THREADPOOL_H
#define MY_TINYMUDUO_BASE_THREADPOOL_H

#include "base/Condition.h"
#include "base/Mutex.h"
#include "base/Thread.h"
#include "base/Types.h"

#include <deque>
#include <vector>

namespace muduo{
    class ThreadPool:noncopyable{
    public:
        typedef std::function<void ()> Task;
        explicit ThreadPool(const string& nameArg = string("ThreadPool"));
        ~ThreadPool();

        void start(int numThreads);
        void stop();

        const string& name()const{
            return name_;
        }
        size_t queueSize()const;

        void run(Task f);
    private:
        bool isFull()const;
        void runInThread();
        Task take();

        mutable MutexLock   mutex_;
        Condition           notEmpty_;
        Condition           notFull_;
        string              name_;
        Task                threadInitCallback_;
        std::vector<std::unique_ptr<muduo::Thread>> threads_;
        std::deque<Task>    queue_;
        size_t              maxQueueSize_;
        bool                running_;
    };




}

#endif //MY_TINYMUDUO_BASE_THREADPOOL_H
