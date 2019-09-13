//
// Created by john on 4/17/19.
//

#include "base/Logging.hpp"
#include "base/CurrentThread.hpp"
#include "base/Timestamp.hpp"
#include "base/StringPiece.hpp"
#include "base/AsyncLogging.hpp"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>
using namespace std;

namespace muduo
{

    __thread char t_errnobuf[512];
    __thread char t_time[64];
    __thread time_t t_lastSecond;

    static pthread_once_t once_done = PTHREAD_ONCE_INIT;
    static std::unique_ptr<AsyncLogging> asyncLogger;
    void once_init()
    {
        asyncLogger.reset(new AsyncLogging(Logger::getFLogname()));
        asyncLogger->start();
    }
    void logToFile(const char* logline,int len)
    {
        pthread_once(&once_done,once_init);
        asyncLogger->append(logline,len);
    }
    const char* strerror_tl(int savedErrno)
    {
        return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
    }
    Logger::LogLevel Logger::g_logLevel = Logger::INFO;
    string Logger::fbasename_ = "hezhiqiang";
    //NUM_LOG_LEVELS的值就是LogLevel的数量
    const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {"TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL "};
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
    stream_ << StringPiece(CurrentThread::tidString(), CurrentThread::tidStringLength());
    stream_ << "|";
    stream_ << StringPiece(LogLevelName[level], 6);
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
        //调用系统函数，得到本地时间（UTC）
        ::gmtime_r(&seconds, &tm_time);

        //将格式化的
        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, //UTC时间转换
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17); (void)len;
    }
    Fmt us(".%06dZ ", microseconds);
    assert(us.length() == 9);
    stream_ << StringPiece(t_time, 17) << StringPiece(us.data(), 9);
}

void Logger::Impl::finish()
{
    //往LogStream的bufffer_写入结束字符
    stream_ << " - " << StringPiece(basename_.data_,basename_.size_) << ':' << line_ << '\n';
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
    //添加结束字符
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    //日志打印到标准输出
    fwrite(buf.data(), 1, buf.length(), stdout);
    logToFile(buf.data(),buf.length());
    //如果LogLevel==FATAL，flush后退出
    if (impl_.level_ == FATAL)
    {
        fflush(stdout);
        abort();
    }
}