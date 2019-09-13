#include"base/FileUtil.hpp"
#include"base/Logging.hpp"
#include"assert.h"
using namespace muduo;

AppendFile::AppendFile(StringArg filename)
    :fp_(::fopen(filename.c_str(),"ae"))
{
    assert(fp_);
    ::setbuffer(fp_,buffer_,sizeof(buffer_));
}
    
AppendFile::~AppendFile()
{
    ::fclose(fp_);
}

void AppendFile::append(const char* logline,const size_t len)
{
    size_t n = write(logline,len);
    size_t remain = len-n;
    while(remain>0)
    {
        size_t x = write(logline+n,remain);
        if(x==0)
        {
            int err = ferror(fp_);
            if(err)
            {
                fprintf(stderr,"AppendFile::append() failed %s\n",strerror_tl(err));
            }
            break;
        }
        n += x;
        remain = len-n;
    }
}
size_t AppendFile::write(const char* logline,size_t len)
{
    return ::fwrite_unlocked(logline,1,len,fp_);
}

void AppendFile::flush()
{
    fflush(fp_);
}