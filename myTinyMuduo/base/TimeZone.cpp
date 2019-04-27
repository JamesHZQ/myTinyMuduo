//
// Created by john on 4/16/19.
//
#include "base/TimeZone.h"
#include "base/noncopyable.h"
#include "base/Date.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <stdio.h>

namespace muduo
{
    namespace detail
    {
        //每个时区的当地时间
        struct Transition
        {
            time_t  gmttime;        //格林威治时间（秒）
            time_t  localtime;      //本地时间
            int     localtimeIdx;   //本地所在时区

            Transition(time_t t, time_t l, int localIdx)
                    : gmttime(t), localtime(l), localtimeIdx(localIdx)
            { }

        };

        struct Comp
        {
            bool compareGmt;

            Comp(bool gmt)
                    : compareGmt(gmt)
            {
            }

            //函数类
            //比较两个时间的先后（根据gmt标志，分别比较格林威治时间和本地时间）
            bool operator()(const Transition& lhs, const Transition& rhs) const
            {
                if (compareGmt)
                    return lhs.gmttime < rhs.gmttime;
                else
                    return lhs.localtime < rhs.localtime;
            }

            bool equal(const Transition& lhs, const Transition& rhs) const
            {
                if (compareGmt)
                    return lhs.gmttime == rhs.gmttime;
                else
                    return lhs.localtime == rhs.localtime;
            }
        };

        //时区信息
        struct Localtime
        {
            time_t  gmtOffset;  //跟格林威治时间相差的秒数
            bool    isDst;      //夏令时？
            int     arrbIdx;    //所在时区缩写索引

            Localtime(time_t offset, bool dst, int arrb)
                    : gmtOffset(offset), isDst(dst), arrbIdx(arrb)
            { }
        };

        //将一天过了seconds秒
        //转换为几点几分几秒
        inline void fillHMS(unsigned seconds, struct tm* utc)
        {
            utc->tm_sec = seconds % 60;
            unsigned minutes = seconds / 60;
            utc->tm_min = minutes % 60;
            utc->tm_hour = minutes / 60;
        }

    }  // namespace detail
    const int kSecondsPerDay = 24*60*60;
}  // namespace muduo

using namespace muduo;
using namespace std;

struct TimeZone::Data
{
    vector<detail::Transition> transitions;
    vector<detail::Localtime> localtimes;
    vector<string> names;
    string abbreviation;
};

namespace muduo
{
    namespace detail
    {

        class File : noncopyable
        {
        public:
            //RAII：构造File对象时打开文件file，File对象析构时关闭文件file
            File(const char* file)
                    : fp_(::fopen(file, "rb"))
            {
            }

            ~File()
            {
                if (fp_)
                {
                    ::fclose(fp_);
                }
            }


            bool valid() const { return fp_; }

            //读取文件file的n个字节并转换为string返回
            string readBytes(int n)
            {
                char buf[n];
                ssize_t nr = ::fread(buf, 1, n, fp_);
                if (nr != n)
                    throw logic_error("no enough data");
                return string(buf, n);
            }

            //读取32位整数
            int32_t readInt32()
            {
                int32_t x = 0;
                ssize_t nr = ::fread(&x, 1, sizeof(int32_t), fp_);
                if (nr != sizeof(int32_t))
                    throw logic_error("bad int32_t data");
                return be32toh(x);
            }

            //读取8位无符号整数
            uint8_t readUInt8()
            {
                uint8_t x = 0;
                ssize_t nr = ::fread(&x, 1, sizeof(uint8_t), fp_);
                if (nr != sizeof(uint8_t))
                    throw logic_error("bad uint8_t data");
                return x;
            }

        private:
            FILE* fp_;
        };

        //读取时区文件
        bool readTimeZoneFile(const char* zonefile, struct TimeZone::Data* data)
        {
            //打开文件zonefile
            File f(zonefile);
            //文件存在
            if (f.valid())
            {
                try
                {
                    //先读四个字节
                    string head = f.readBytes(4);
                    //一定得是“TZif”
                    if (head != "TZif")
                        throw logic_error("bad head");
                    //再读一个字节，表示文件版本
                    string version = f.readBytes(1);
                    //再读15字节
                    f.readBytes(15);

                    //接着读6个32位整数的信息
                    int32_t isgmtcnt = f.readInt32();
                    int32_t isstdcnt = f.readInt32();
                    int32_t leapcnt = f.readInt32();
                    int32_t timecnt = f.readInt32();
                    int32_t typecnt = f.readInt32();
                    int32_t charcnt = f.readInt32();

                    vector<int32_t> trans;
                    vector<int> localtimes;
                    //申请timecnt空间
                    trans.reserve(timecnt);
                    for (int i = 0; i < timecnt; ++i)
                    {
                        //格林威治时间
                        trans.push_back(f.readInt32());
                    }

                    for (int i = 0; i < timecnt; ++i)
                    {
                        uint8_t local = f.readUInt8();
                        localtimes.push_back(local);
                    }

                    //添加时区信息
                    for (int i = 0; i < typecnt; ++i)
                    {
                        int32_t gmtoff = f.readInt32();
                        uint8_t isdst = f.readUInt8();
                        uint8_t abbrind = f.readUInt8();

                        data->localtimes.push_back(Localtime(gmtoff, isdst, abbrind));
                    }

                    //不同时区的当地时间
                    for (int i = 0; i < timecnt; ++i)
                    {
                        //localIdx所在时区
                        int localIdx = localtimes[i];
                        //当地时间=格林威治时间+当地所在时区与格林威治相差的秒数
                        time_t localtime = trans[i] + data->localtimes[localIdx].gmtOffset;
                        data->transitions.push_back(Transition(trans[i], localtime, localIdx));
                    }

                    data->abbreviation = f.readBytes(charcnt);

                    // leapcnt
                    for (int i = 0; i < leapcnt; ++i)
                    {
                        // int32_t leaptime = f.readInt32();
                        // int32_t cumleap = f.readInt32();
                    }
                    // FIXME
                    (void) isstdcnt;
                    (void) isgmtcnt;
                }
                catch (logic_error& e)
                {
                    fprintf(stderr, "%s\n", e.what());
                }
            }
            return true;
        }

        //查看所在时区
        const Localtime* findLocaltime(const TimeZone::Data& data, Transition sentry, Comp comp)
        {
            const Localtime* local = NULL;

            if (data.transitions.empty() || comp(sentry, data.transitions.front()))
            {
                // FIXME: should be first non dst time zone
                local = &data.localtimes.front();
            }
            else
            {
                vector<Transition>::const_iterator
                    transI = lower_bound(data.transitions.begin(), data.transitions.end(), sentry, comp);
                if (transI != data.transitions.end())
                {
                    if (!comp.equal(sentry, *transI))
                    {
                        assert(transI != data.transitions.begin());
                        --transI;
                    }
                    local = &data.localtimes[transI->localtimeIdx];
                }
                else
                {
                    // FIXME: use TZ-env
                    local = &data.localtimes[data.transitions.back().localtimeIdx];
                }
            }

            return local;
        }

    }  // namespace detail
}  // namespace muduo


TimeZone::TimeZone(const char* zonefile)
        : data_(new TimeZone::Data)
{
    //读取文件失败要释放data_占用的内存
    if (!detail::readTimeZoneFile(zonefile, data_.get()))
    {
        data_.reset();
    }
}

TimeZone::TimeZone(int eastOfUtc, const char* name)
        : data_(new TimeZone::Data)
{
    //添加一个时区
    data_->localtimes.push_back(detail::Localtime(eastOfUtc, false, 0));
    data_->abbreviation = name;
}

struct tm TimeZone::toLocalTime(time_t seconds) const
{
    struct tm localTime;
    memZero(&localTime, sizeof(localTime));
    assert(data_ != NULL);
    const Data& data(*data_);

    detail::Transition sentry(seconds, 0, 0);

    //根据格林威治时间，查看所在时区                                                                //传入函数类对象
    const detail::Localtime* local = findLocaltime(data, sentry, detail::Comp(true));

    //得到本地时间
    if (local)
    {
        time_t localSeconds = seconds + local->gmtOffset;
        ::gmtime_r(&localSeconds, &localTime); // FIXME: fromUtcTime
        localTime.tm_isdst = local->isDst;
        localTime.tm_gmtoff = local->gmtOffset;
        localTime.tm_zone = &data.abbreviation[local->arrbIdx];
    }

    return localTime;
}

time_t TimeZone::fromLocalTime(const struct tm& localTm) const
{
    assert(data_ != NULL);
    const Data& data(*data_);

    struct tm tmp = localTm;
    time_t seconds = ::timegm(&tmp); // FIXME: toUtcTime
    detail::Transition sentry(0, seconds, 0);
    const detail::Localtime* local = findLocaltime(data, sentry, detail::Comp(false));
    if (localTm.tm_isdst)
    {
        struct tm tryTm = toLocalTime(seconds - local->gmtOffset);
        if (!tryTm.tm_isdst
            && tryTm.tm_hour == localTm.tm_hour
            && tryTm.tm_min == localTm.tm_min)
        {
            // FIXME: HACK
            seconds -= 3600;
        }
    }
    return seconds - local->gmtOffset;
}

struct tm TimeZone::toUtcTime(time_t secondsSinceEpoch, bool yday)
{
    struct tm utc;
    memZero(&utc, sizeof(utc));
    utc.tm_zone = "GMT";
    int seconds = static_cast<int>(secondsSinceEpoch % kSecondsPerDay);
    int days = static_cast<int>(secondsSinceEpoch / kSecondsPerDay);
    if (seconds < 0)
    {
        seconds += kSecondsPerDay;
        --days;
    }
    detail::fillHMS(seconds, &utc);
    //距离基准时间的天数
    Date date(days + Date::kJulianDayOf1970_01_01);
    Date::YearMonthDay ymd = date.yearMonthDay();
    //转换为utc年月日，year从1900开始，mon：0-11
    utc.tm_year = ymd.year - 1900;
    utc.tm_mon = ymd.month - 1;
    utc.tm_mday = ymd.day;
    utc.tm_wday = date.weekDay();

    if (yday)
    {
        Date startOfYear(ymd.year, 1, 1);
        //今天距离基准日期的天数减去，今年1月1日距离基准日期的天数，得到今天是今年的第几天
        utc.tm_yday = date.julianDayNumber() - startOfYear.julianDayNumber();
    }
    return utc;
}

time_t TimeZone::fromUtcTime(const struct tm& utc)
{
    return fromUtcTime(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                       utc.tm_hour, utc.tm_min, utc.tm_sec);
}

time_t TimeZone::fromUtcTime(int year, int month, int day,
                             int hour, int minute, int seconds)
{
    Date date(year, month, day);
    int secondsInDay = hour * 3600 + minute * 60 + seconds;
    time_t days = date.julianDayNumber() - Date::kJulianDayOf1970_01_01;
    return days * kSecondsPerDay + secondsInDay;
}

