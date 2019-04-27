//
// Created by john on 4/17/19.
//

#include "base/Logging.h"
#include "base/CurrentThread.h"
#include "base/TimeZone.h"
#include "base/Timestamp.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

namespace muduo
{

    __thread char t_errnobuf[512];
    __thread char t_time[64];
    __thread time_t t_lastSecond;

    const char* strerror_tl(int savedErrno)
    {
        //将错误写到t_errnobuf里
        return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
    }

    Logger::LogLevel initLogLevel()
    {
        if (::getenv("MUDUO_LOG_TRACE"))
            return Logger::TRACE;
        else if (::getenv("MUDUO_LOG_DEBUG"))
            return Logger::DEBUG;
        else
            return Logger::INFO;
    }

    Logger::LogLevel g_logLevel = initLogLevel();

    //NUM_LOG_LEVELS的值就是LogLevel的数量
    const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
            {
                    "TRACE ",
                    "DEBUG ",
                    "INFO  ",
                    "WARN  ",
                    "ERROR ",
                    "FATAL ",
            };

// helper class for known string length at compile time
    class T
    {
    public:
        T(const char* str, unsigned len)
                :str_(str),
                 len_(len)
        {
            assert(strlen(str) == len_);
        }

        const char* str_;
        const unsigned len_;
    };

    //把T类型的字符串写到LogStream
    inline LogStream& operator<<(LogStream& s, T v)
    {
        s.append(v.str_, v.len_);
        return s;
    }
    //将文件名写到LogStream
    inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
    {
        s.append(v.data_, v.size_);
        return s;
    }

    //默认讲输出信息写到标准输出
    void defaultOutput(const char* msg, int len)
    {
        size_t n = fwrite(msg, 1, len, stdout);
        //FIXME check n
        (void)n;
    }

    //默认flush标准缓冲区
    void defaultFlush()
    {
        fflush(stdout);
    }

    //将输出和flush回调函数都设为默认函数，后面可通过setxxx修改
    Logger::OutputFunc g_output = defaultOutput;
    Logger::FlushFunc g_flush = defaultFlush;
    TimeZone g_logTimeZone;

}  // namespace muduo

using namespace muduo;

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
        : time_(Timestamp::now()),
          stream_(),
          level_(level),
          line_(line),
          basename_(file)
{
    formatTime();
    //将当将线程的tid存到t_cachedTid
    CurrentThread::tid();
    //往LogStream里写入线程tid及其长度
    stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
    stream_ << T(LogLevelName[level], 6);
    if (savedErrno != 0)
    {
        //往LogStream里写入errno
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

void Logger::Impl::formatTime()
{
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
    if (seconds != t_lastSecond)
    {
        t_lastSecond = seconds;
        struct tm tm_time;
        if (g_logTimeZone.valid())
        {
            //得到当前本地时间
            //是否还应该调用fromLocalTime得到UTC时间
            tm_time = g_logTimeZone.toLocalTime(seconds);
        }
        else
        {
            //调用系统函数，得到本地时间（UTC）
            ::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
        }

        //将格式化的
        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, //UTC时间转换
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17); (void)len;
    }

    if (g_logTimeZone.valid())
    {
        //将.+6位有效数字写到us的buf_
        Fmt us(".%06d ", microseconds);
        assert(us.length() == 8);
        //相同秒数的时间只有微秒数是不同的
        stream_ << T(t_time, 17) << T(us.data(), 8);
    }
    else
    {
        Fmt us(".%06dZ ", microseconds);
        assert(us.length() == 9);
        stream_ << T(t_time, 17) << T(us.data(), 9);
    }
}

void Logger::Impl::finish()
{
    //往LogStream的bufffer_写入结束字符
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line)
        : impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
        : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
        : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort)
        : impl_(toAbort?FATAL:ERROR, errno, file, line)
{
}

Logger::~Logger()
{
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_output(buf.data(), buf.length());
    //如果LogLevel==FATAL，flush后退出
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}

void Logger::setTimeZone(const TimeZone& tz)
{
    g_logTimeZone = tz;
}