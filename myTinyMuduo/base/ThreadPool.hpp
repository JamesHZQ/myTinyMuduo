//
// Created by john on 4/17/19.
//

#ifndef MY_TINYMUDUO_BASE_THREADPOOL_HPP
#define MY_TINYMUDUO_BASE_THREADPOOL_HPP

#include "base/Condition.hpp"
#include "base/Mutex.hpp"
#include "base/Thread.hpp"

#include <deque>
#include <vector>
#include <string>

namespace muduo{
    class ThreadPool:noncopyable{
    public:
        typedef std::function<void ()> Task;
        explicit ThreadPool(const std::string& nameArg = std::string("ThreadPool"));
        ~ThreadPool();

        void start(int numThreads);
        void stop();

        const std::string& name()const{
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
        std::string              name_;
        Task                threadInitCallback_;
        std::vector<std::unique_ptr<muduo::Thread>> threads_;
        std::deque<Task>    queue_;
        size_t              maxQueueSize_;
        bool                running_;
    };




}

#endif //MY_TINYMUDUO_BASE_THREADPOOL_H
