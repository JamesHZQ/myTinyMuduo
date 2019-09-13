#ifndef MY_TINYMUDUO_BASE_FILEUTIL_HPP
#define MY_TINYMUDUO_BASE_FILEUTIL_HPP

#include"base/noncopyable.hpp"
#include"base/StringPiece.hpp"

namespace muduo{
    class AppendFile :noncopyable
    {
    private:
        FILE* fp_;
        char buffer_[64*1024];
        size_t write(const char* logline,size_t len);

    public:
        explicit AppendFile(StringArg filename);
        ~AppendFile();
        void append(const char* logfile,const size_t len);
        void flush();
    };
    
    
}

#endif