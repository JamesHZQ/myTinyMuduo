//
// Created by john on 4/19/19.
//

#ifndef MY_TINYMUDUO_NET_POLLPOLLER_HPP
#define MY_TINYMUDUO_NET_POLLPOLLER_HPP

#include "net/Poller.hpp"

#include <vector>

struct pollfd;

namespace muduo{
    namespace net{
        //使用poll()实现IO复用
        class PollPoller:public Poller{
        public:
            PollPoller(EventLoop* loop);
            ~PollPoller()override ;

            Timestamp poll(int timeoutMs,ChannelList* activeChannels) override;
            void updateChannel(Channel* channel)override ;
            void removeChannel(Channel* channel)override ;
        private:
            void fillActiveChannels(int numEvents, ChannelList* activeChannels)const;

            typedef std::vector<struct pollfd> PollFdList;
            PollFdList pollfds_;
        };
    }
}

#endif //MY_TINYMUDUO_NET_POLLPOLLER_H





















