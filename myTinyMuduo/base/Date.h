//
// Created by john on 4/16/19.
//

#ifndef MY_TINYMUDUO_BASE_DATE_H
#define MY_TINYMUDUO_BASE_DATE_H
#include "base/copyable.h"
#include "base/Types.h"

struct tm;

namespace muduo
{

///
/// Date in Gregorian calendar.
///
/// This class is immutable.
/// It's recommended to pass it by value, since it's passed in register on x64.
///
    class Date : public muduo::copyable
        // public boost::less_than_comparable<Date>,
        // public boost::equality_comparable<Date>
    {
    public:

        struct YearMonthDay
        {
            int year; // [1900..2500]
            int month;  // [1..12]
            int day;  // [1..31]
        };

        static const int kDaysPerWeek = 7;
        //1970-01-01距离基准日期的天数
        static const int kJulianDayOf1970_01_01;

        //不可用（非法）日期，用于判断
        Date()
                : julianDayNumber_(0)
        {}

        //按年月日生成Date对象
        Date(int year, int month, int day);

        //按绝对日期生成Date，长期纪日法
        explicit Date(int julianDayNum)
                : julianDayNumber_(julianDayNum)
        {}

        //按结构体tm生成Date
        explicit Date(const struct tm&);

        // default copy/assignment/dtor are Okay

        void swap(Date& that)
        {
            std::swap(julianDayNumber_, that.julianDayNumber_);
        }

        //判断Date是否有效
        bool valid() const { return julianDayNumber_ > 0; }

        //生成打印格式
        string toIsoString() const;

        //返回年月日结构体
        struct YearMonthDay yearMonthDay() const;

        int year() const
        {
            return yearMonthDay().year;
        }

        int month() const
        {
            return yearMonthDay().month;
        }

        int day() const
        {
            return yearMonthDay().day;
        }

        // [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
        int weekDay() const
        {
            return (julianDayNumber_+1) % kDaysPerWeek;
        }

        int julianDayNumber() const { return julianDayNumber_; }

    private:
        //距离基准日期的天数
        int julianDayNumber_;
    };

    inline bool operator<(Date x, Date y)
    {
        return x.julianDayNumber() < y.julianDayNumber();
    }

    inline bool operator==(Date x, Date y)
    {
        return x.julianDayNumber() == y.julianDayNumber();
    }

}  // namespace muduo

#endif //MY_TINYMUDUO_BASE_DATE_H
