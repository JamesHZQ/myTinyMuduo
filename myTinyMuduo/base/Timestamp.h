//
// Created by john on 4/16/19.
//

#ifndef MY_TINYMUDUO_BASE_TIMESTAMP_H
#define MY_TINYMUDUO_BASE_TIMESTAMP_H

#include "base/copyable.h"
#include "base/Types.h"
#include <boost/operators.hpp>
namespace muduo{
//继承了boost库下的equality_comparable和less_than_comparable
//可根据<和等于运算符自动生成>,>=,<=,!=
//1970-01-01为时间戳的基准时间
class Timestamp:public muduo::copyable,
                public boost::equality_comparable<Timestamp>,
                public boost::less_than_comparable<Timestamp>
{
public:
    //构造1个非法的时间戳
    Timestamp()
        :microSecondsSinceEpoch_(0)
    {}
    //根据微秒数构造时间戳（距离基准时间的微妙数）
    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        :microSecondsSinceEpoch_(microSecondsSinceEpochArg)
    {}

    void swap(Timestamp& that){
        std::swap(microSecondsSinceEpoch_,that.microSecondsSinceEpoch_);
    }
    // 使用默认的拷贝和析构函数

    string toString() const;
    string toFormattedString(bool showMicroseconds = true) const;

    //根据微秒数是否大于零判断时间戳是否合法
    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    // for internal usage.
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    //返回距离基准时间的秒数
    time_t secondsSinceEpoch() const
    { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

    ///
    /// Get time of now.
    ///
    //当前时间距离基准时间的秒数
    static Timestamp now();
    //反会非法时间
    static Timestamp invalid()
    {
        return Timestamp();
    }
    //
    static Timestamp fromUnixTime(time_t t)
    {
        return fromUnixTime(t, 0);
    }
    //返回距离基准时间t秒microseconds微秒的时间戳
    static Timestamp fromUnixTime(time_t t, int microseconds)
    {
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
    }

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
//返回两个时间戳的时间差（单位：秒）
inline double timeDifference(Timestamp high, Timestamp low){
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}
//返回一个时间戳seconds秒后对应的时间戳
inline Timestamp addTime(Timestamp timestamp, double seconds){
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}
}

#endif //MY_TINYMUDUO_BASE_TIMESTAMP_H
