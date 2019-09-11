//
// Created by john on 4/24/19.
//

#ifndef MY_TINYMUDUO_NET_EVENTLOOPTHREAD_HPP
#define MY_TINYMUDUO_NET_EVENTLOOPTHREAD_HPP

#include "base/Condition.hpp"
#include "base/Mutex.hpp"
#include "base/Thread.hpp"

#include<string>
namespace muduo{
    namespace net{
        class EventLoop;
        //EventLoopThread对象创建时，创建一个新线程并在其中创建EventLoop对象
        class EventLoopThread:noncopyable{
        public:
            typedef std::function<void(EventLoop*)> ThreadInitCallback;
            EventLoopThread(const ThreadInitCallback&cb = ThreadInitCallback(),const std::string& name=std::string());
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
