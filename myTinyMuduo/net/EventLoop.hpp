//
// Created by john on 4/18/19.
//

#ifndef MY_TINYMUDUO_NET_EVENTLOOP_HPP
#define MY_TINYMUDUO_NET_EVENTLOOP_HPP

#include "base/Mutex.hpp"
#include "base/CurrentThread.hpp"
#include "base/Timestamp.hpp"
#include "net/TimerId.hpp"
#include "net/Callbacks.hpp"

#include <atomic>
#include <functional>
#include <vector>
#include <any>

namespace muduo{
    namespace net{
        class Channel;
        class Poller;
        class TimerQueue;

        class EventLoop: noncopyable{
        public:
            typedef std::function<void()> Functor;

            EventLoop();
            ~EventLoop();

            void loop();
            void quit();

            Timestamp pollReturnTime()const{ return pollReturnTime_;}
            int64_t iteration()const { return iteration_;}

            //任务队列（线程切换）
            void runInLoop(Functor cb);
            void queueInLoop(Functor cb);
            size_t queueSize()const;

            //定时器
            TimerId runAt(Timestamp timer, TimerCallback cb);
            TimerId runAfter(double delay, TimerCallback cb);
            TimerId runEvery(double interval,TimerCallback cb);
            void cancel(TimerId timerId);

            //事件处理
            void wakeup();
            void updateChannel(Channel* channel);
            void removeChannel(Channel* channel);
            bool hasChannel(Channel* channel);

            //判断当前线程是否是本事件循环所属的线程（一个线程至多拥有一个事件循环EventLoop）
            bool isInloopThread()const{ return threadId_==CurrentThread::tid();}
            void assertInLoopThread(){
                if(!isInloopThread()){
                    abortNotInloopThread();
                }
            }

            void setContex(const std::any& context){contex_=context;}
            const std::any& getContext()const{return contex_;}
            std::any* getMutableContext(){return &contex_;}

            static EventLoop* getEventLoopOfCurrentThread();

        private:
            void abortNotInloopThread();
            void handleRead();
            void doPendingFunctors();

            void printActiveChannels()const;

            typedef std::vector<Channel*>ChannelList;

            bool                        looping_;
            std::atomic<bool>           quit_;              //原子量
            bool                        eventHandling_;
            bool                        callingPendingFunctors_;
            int64_t                     iteration_;
            const pid_t                 threadId_;
            Timestamp                   pollReturnTime_;
            std::unique_ptr<Poller>     poller_;            //***
            std::unique_ptr<TimerQueue> timerQueue_;        //***
            int                         wakeupFd_;

            std::unique_ptr<Channel>    wakeupChannel_;
            std::any                  contex_;

            ChannelList                 activeChannels_;    //***
            Channel*                    currentActiveChannel_;
            mutable MutexLock           mutex_;
            std::vector<Functor>        pendingFunctors_;   //***

        };
    }
}

#endif //MY_TINYMUDUO_NET_EVENTLOOP_H
