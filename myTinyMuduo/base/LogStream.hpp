//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_LOGSTREAM_HPP
#define MY_TINYMUDUO_BASE_LOGSTREAM_HPP

#include "base/noncopyable.hpp"
#include "base/StringPiece.hpp"

#include <assert.h>
#include <string.h>
#include <string>

namespace muduo{
    namespace detail{
        const int kSmallBuffer = 4000;
        const int kLargeBuffer = 4000*1000;
        //FixedBuffer末尾没有'\0'中止符
        template <int SIZE>
        class FixedBuffer:noncopyable{
        public:
            FixedBuffer(): cur_(data_){
                bzero();
            }
            ~FixedBuffer()=default;

            void append(const char* buf,size_t len){
                if(static_cast<size_t >(avail())>len){
                    //如果可用空间足够，将buf复制到cur_之后的位置
                    memcpy(cur_,buf,len);
                    //cur_后移buf的长度
                    cur_ += len;
                }
            }
            //返回字符串首地址
            const char* data()const{return data_;}
            int length()const{return static_cast<int>(cur_-data_);}
            char* current(){return cur_;}
            int avail()const{return static_cast<int>(end()-cur_);}
            void add(size_t len){cur_+=len;}
            void reset(){cur_=data_;}
            void bzero(){memset(data_, 0, sizeof(data_));}
            //返回string或StringPiece
            std::string toString()const{return std::string(data_,length());}
            StringPiece toStringPiece()const{return StringPiece(data_,length());}

        private:
            //返回尾后指针
            const char* end()const{ return data_+ sizeof(data_);}
            char    data_[SIZE];
            char*   cur_;
        };
    }
    class LogStream:noncopyable{
    public:
        typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;
        LogStream& operator<<(bool v){
            buffer_.append(v?"1":"0",1);
            return *this;
        }
        LogStream& operator<<(short);
        LogStream& operator<<(unsigned short);
        LogStream& operator<<(int);
        LogStream& operator<<(unsigned int);
        LogStream& operator<<(long);
        LogStream& operator<<(unsigned long);
        LogStream& operator<<(long long);
        LogStream& operator<<(unsigned long long);
        LogStream& operator<<(const void*);
        LogStream& operator<<(float v){
            *this<< static_cast<double>(v);
            return *this;
        }
        LogStream& operator<<(double);
        LogStream& operator<<(char v){
            buffer_.append(&v,1);
            return *this;
        }
        LogStream& operator<<(const char* str){
            if(str){
                buffer_.append(str,strlen(str));
            }else{
                buffer_.append("(null)",6);
            }
            return *this;
        }
        LogStream&operator<<(const unsigned char* str){
            return operator<<(reinterpret_cast<const char*>(str));
        }
        LogStream&operator<<(const std::string& v){
            buffer_.append(v.c_str(),v.size());
            return *this;
        }
        LogStream&operator<<(const StringPiece& v){
            buffer_.append(v.data(),v.size());
            return *this;
        }
        LogStream&operator<<(const Buffer& v){
            *this<<v.toStringPiece();
            return *this;
        }

        void append(const char* data, int len) { buffer_.append(data, len); }

        const Buffer& buffer()const{return buffer_;}
        void resetBuffer(){buffer_.reset();}


    private:
        template <typename T>
        void formatInteger(T);
        //容量4000个字节
        Buffer buffer_;
        static const int kMaxNumericSize = 32;
    };

}

#endif //MY_TINYMUDUO_BASE_LOGSTREAM_H
