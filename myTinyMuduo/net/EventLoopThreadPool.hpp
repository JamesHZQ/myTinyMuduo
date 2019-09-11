//
// Created by john on 4/25/19.
//

#ifndef MY_TINYMUDUO_NET_EVENTLOOPTHREADPOOL_HPP
#define MY_TINYMUDUO_NET_EVENTLOOPTHREADPOOL_HPP

#include "base/noncopyable.hpp"

#include <functional>
#include <memory>
#include <vector>
#include <string>

namespace muduo{
    namespace net{
        class EventLoop;
        class EventLoopThread;

        class EventLoopThreadPool:noncopyable{
        public:
            typedef std::function<void(EventLoop*)> ThreadInitCallback;

            EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg);
            ~EventLoopThreadPool();
            void setThreadNum(int numThreads){numThreads_ = numThreads;}
            void start(const ThreadInitCallback& cb = ThreadInitCallback());

            //round-robin(轮转调度)
            EventLoop* getNextLoop();

            EventLoop* getLoopForHash(size_t hashCode);

            std::vector<EventLoop*> getAllLoops();

            bool started()const{ return started_;}
            const std::string& name()const{ return name_;}

        private:
            EventLoop*  baseLoop_;
            std::string      name_;
            bool        started_;
            int         numThreads_;
            int         next_;
            std::vector<std::unique_ptr<EventLoopThread>> threads_;
            std::vector<EventLoop*> loops_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_EVENTLOOPTHREADPOOL_H
