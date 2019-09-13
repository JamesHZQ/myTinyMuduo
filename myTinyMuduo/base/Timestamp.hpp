//
// Created by john on 4/16/19.
//

#ifndef MY_TINYMUDUO_BASE_TIMESTAMP_HPP
#define MY_TINYMUDUO_BASE_TIMESTAMP_HPP

#include "base/copyable.hpp"
#include <string>
namespace muduo{
//1970-01-01为时间戳的基准时间
    class Timestamp:public muduo::copyable{
    public:
        Timestamp():microSecondsSinceEpoch_(0){}
        explicit Timestamp(int64_t microSecondsSinceEpochArg):microSecondsSinceEpoch_(microSecondsSinceEpochArg){}

        std::string toString() const;
        std::string toFormattedString(bool showMicroseconds = true) const;

        //根据微秒数是否大于零判断时间戳是否合法
        bool valid() const { 
            return microSecondsSinceEpoch_ > 0; 
        }

        //返回时间（微秒/秒）
        int64_t microSecondsSinceEpoch() const { 
            return microSecondsSinceEpoch_; 
        }
        time_t secondsSinceEpoch() const{ 
            return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); 
        }

        //返回当前时间戳
        static Timestamp now();
        static const int kMicroSecondsPerSecond = 1000 * 1000;
    private:
        int64_t microSecondsSinceEpoch_;
};
    inline bool operator<(Timestamp lhs, Timestamp rhs){
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    inline bool operator==(Timestamp lhs, Timestamp rhs){
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }
    //返回两个时间戳的时间差（秒）
    inline double timeDifference(Timestamp high, Timestamp low){
        int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
    }
    //返回一个时间戳seconds秒后的时间戳
    inline Timestamp addTime(Timestamp timestamp, double seconds){
        int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
        return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
    }
}

#endif //MY_TINYMUDUO_BASE_TIMESTAMP_H
