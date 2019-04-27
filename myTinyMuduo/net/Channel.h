//
// Created by john on 4/18/19.
//

#ifndef MY_TINYMUDUO_NET_CHANNEL_H
#define MY_TINYMUDUO_NET_CHANNEL_H

#include "base/noncopyable.h"
#include "base/Timestamp.h"

#include <functional>
#include <memory>
//Channel对应1个文件描述符（套接字，timerfd，一般文件描述符），但并不拥有这个文件描述符
//Channel使用回调函数的方法，封装文件描述符的可读，可写，关闭，和错误的操作
//Channel不属于EventLoop也不属于Poller
namespace muduo{
    namespace net{
        class EventLoop;
        class Channel:noncopyable{
        public:
            typedef std::function<void()> EventCallback;
            typedef std::function<void(Timestamp)> ReadEventCallback;

            Channel(EventLoop* loop,int fd);
            ~Channel();

            //处理回调事件，一般由Poller通过EventLoop调用
            void handleEvent(Timestamp receiveTime);

            //设置四种回调函数
            void setReadCallback(ReadEventCallback cb)
            {readCallback_ = std::move(cb);}
            void setWriteCallback(EventCallback cb)
            {writeCallback_ = std::move(cb);}
            void setCloseCallback(EventCallback cb)
            {closeCallback_ = std::move(cb);}
            void setErrorCallback(EventCallback cb)
            {errorCallback_ = std::move(cb);}

            //绑定1个对象，阻止这个对象在handleEvent中被销毁
            //一般是tie一个TcpConnection对象
            void tie(const std::shared_ptr<void>& );


            int fd()const{return fd_;}
            int events()const{return events_;}
            //进行poll或epoll_wait调用后，设置revents_记录发生的事件
            void set_revents(int revt){ revents_ = revt;}
            //是否无感兴趣的事件（没有监听任何事件）
            bool isNoneEvent()const{return events_ == kNoneEvent;}

            //启动或停止监听各种类型的事件
            void enableReading(){events_ |= kReadEvent;update();}
            void disableReading(){events_ &= ~kReadEvent;update();}
            void enableWritting(){events_ |= kWriteEvent;update();}
            void disableWriting(){events_ &= ~kWriteEvent;update();}
            void disableAll(){events_ = kNoneEvent;update();}
            //判断Channel是否正在监听读写事件
            bool isWriting()const{return events_&kWriteEvent;}
            bool isReading()const{return events_&kReadEvent;}

            int index(){return index_;}
            void set_index(int idx){index_ = idx;}

            string reventsToString()const;
            string eventsToString()const;

            //是否将logHup_事件写入日志
            void doNotLogHup(){logHup_ = false;}
            EventLoop* owerLoop(){return loop_;}

            //从EventLoop中移除该Channnel
            void remove();

        private:
            static string eventsToString(int fd, int ev);

            void update();
            void handleEventWithGuard(Timestamp receiveTime);

            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;

            EventLoop*  loop_;
            const int   fd_;
            int         events_;
            int         revents_;
            int         index_;
            bool        logHup_;

            std::weak_ptr<void> tie_;
            bool                tied_;
            bool                eventHandling_;
            bool                addedToloop_;
            ReadEventCallback   readCallback_;
            EventCallback       writeCallback_;
            EventCallback       closeCallback_;
            EventCallback       errorCallback_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_CHANNEL_H
