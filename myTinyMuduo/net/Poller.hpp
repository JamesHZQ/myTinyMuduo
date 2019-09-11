//
// Created by john on 4/19/19.
//

#ifndef MY_TINYMUDUO_NET_POLLER_HPP
#define MY_TINYMUDUO_NET_POLLER_HPP

#include "base/Timestamp.hpp"
#include "net/EventLoop.hpp"

#include <map>
#include <vector>

namespace muduo{
    namespace net{
        class Channel;
        class Poller:noncopyable{
        public:
            typedef std::vector<Channel*> ChannelList;

            Poller(EventLoop* loop);
            virtual ~Poller();

            virtual Timestamp poll(int timeoutMs,ChannelList* activeChannels) = 0;

            virtual void updateChannel(Channel* channel) = 0;

            virtual void removeChannel(Channel* channel) = 0;

            virtual bool hasChannel(Channel* channel)const;

            static Poller* newDefaultPoller(EventLoop* loop);
            void asserInLoopThread()const{
                ownerLoop_->assertInLoopThread();
            }

        protected:
            typedef std::map<int,Channel*> ChannelMap;
            ChannelMap channels_;
        private:
            EventLoop* ownerLoop_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_POLLER_H
