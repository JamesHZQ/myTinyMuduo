#include "LogFile.hpp"
#include "FileUtil.hpp"

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

using namespace std;
using namespace muduo;

LogFile::LogFile(const string& basename,int flushInterval,int flushEveryN)
    :   basename_(basename),
        mutex_(),
        flushInterval_(flushInterval),  //flush的时间间隔
        flushEveryN_(flushEveryN),      //flush的条数间隔
        count_(0),
        lastRoll_(0),                   //上次归档时间
        lastFlush_(0)                   //上次flush的时间
{
    assert(basename_.find('/')==string::npos);
    rollFile();
}

void LogFile::append(const char* logline,int len)
{
    MutexLockGuard lock(mutex_);
    append_unlocked(logline,len);
}
void LogFile::flush()
{
    MutexLockGuard lock(mutex_);
    file_->flush();
}
void LogFile::append_unlocked(const char* logline,int len){
    file_->append(logline,len);
    time_t now = ::time(NULL);
    if(++count_>=flushEveryN_||now-lastFlush_>flushInterval_){
        time_t nowLogday = now/kRollPerSeconds_*kRollPerSeconds_;
        time_t lastLogday = lastRoll_/kRollPerSeconds_*kRollPerSeconds_;
        if(nowLogday!=lastLogday)
        {
            rollFile();
        }
        else 
        {
            if(count_>=flushEveryN_)
            {
               count_=0; 
            }
                
            if(now-lastFlush_>flushInterval_)
            {
                lastFlush_ = now;
            }
            file_->flush(); 
        }
    }

}
bool LogFile::rollFile()
{
    time_t now = 0;
    string filename = getLogFileName(basename_,&now);
    if(now>lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ = now;
        file_.reset(new AppendFile(filename));
        return true;
    }
    return false;
}

string LogFile::getLogFileName(const string& basename,time_t* now)
{
    string filename(basename);
    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    gmtime_r(now,&tm);
    strftime(timebuf,sizeof(timebuf),".%Y%m%d-%H%M%S.",&tm);
    filename += timebuf;
    char hostnamebuf[256];
    if(::gethostname(hostnamebuf,sizeof(hostnamebuf))==0)
    {
        hostnamebuf[255]='\0';
        filename += hostnamebuf;
    }
    else
    {
        filename+="unknownhost";
    }
    char pidbuf[32];
    snprintf(pidbuf,sizeof(pidbuf),".%d",::getpid());
    filename+=pidbuf;
    filename+=".log";
    return filename;
}