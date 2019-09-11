//
// Created by john on 4/19/19.
//

#include "net/Timer.hpp"

using namespace muduo;
using namespace muduo::net;

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(muduo::Timestamp now) {
    if(repeat_){
        //如果需要重复将时间设为下次过期的时间
        expiration_ = addTime(now,interval_);
    }else{
        //如果不需要重复，将过期时间设为不可用
        expiration_ = Timestamp::invalid();
    }
}