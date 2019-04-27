//
// Created by john on 4/24/19.
//

#ifndef MY_TINYMUDUO_NET_EVENTLOOPTHREAD_H
#define MY_TINYMUDUO_NET_EVENTLOOPTHREAD_H

#include "base/Condition.h"
#include "base/Mutex.h"
#include "base/Thread.h"

namespace muduo{
    namespace net{
        class EventLoop;
        //用于开启一个线程执行时间循环
        class EventLoopThread:noncopyable{
        public:
            typedef std::function<void(EventLoop*)> ThreadInitCallback;
            EventLoopThread(const ThreadInitCallback&cb = ThreadInitCallback(),const string& name = string());
            ~EventLoopThread();
            EventLoop* startLoop();

        private:
            void threadFunc();

            EventLoop*          loop_;
            bool                exiting_;
            Thread              thread_;
            MutexLock           mutex_;
            Condition           cond_;
            ThreadInitCallback  callback_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_EVENTLOOPTHREAD_H
