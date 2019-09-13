#ifndef MY_TINYMUDUO_BASE_LOGFILE_HPP
#define MY_TINYMUDUO_BASE_LOGFILE_HPP

#include"base/noncopyable.hpp"
#include"base/Mutex.hpp"
#include"base/FileUtil.hpp"

#include<string>
#include<memory>

namespace muduo
{
    class LogFile:noncopyable
    {
    private:
        void append_unlocked(const char* logline,int len);
        static std::string getLogFileName(const std::string& basename,time_t* now);

        const std::string basename_;        
        mutable MutexLock mutex_;
        int flushInterval_;
        int flushEveryN_;
        int count_;
        time_t lastRoll_;
        time_t lastFlush_;
        std::unique_ptr<AppendFile> file_;
        const static int kRollPerSeconds_ = 60*60*24;
    public:
        LogFile(const std::string& basename,int flushInterval=3,int flushEveryN = 1024);
        ~LogFile()=default;

        void append(const char* logline,int len);
        void flush();
        bool rollFile();

    };
}

#endif