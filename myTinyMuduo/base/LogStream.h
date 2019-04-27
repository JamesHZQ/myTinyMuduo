//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_LOGSTREAM_H
#define MY_TINYMUDUO_BASE_LOGSTREAM_H

#include "base/noncopyable.h"
#include "base/StringPiece.h"
#include "base/Types.h"

#include <assert.h>
#include <string.h>

namespace muduo{
    namespace detail{
        const int kSmallBuffer = 4000;
        const int kLargeBuffer = 4000*1000;
        template <int SIZE>
        class FixedBuffer:noncopyable{
        public:
            FixedBuffer()
                : cur_(data_){
                setCookie(cookieStart);
            }
            ~FixedBuffer(){
                setCookie(cookieEnd);
            }

            void append(const char* buf,size_t len){
                if(implicit_cast<size_t >(avail())>len){
                    //如果可用空间足够，将buf复制到cur_之后的位置
                    memcpy(cur_,buf,len);
                    //cur_后移buf的长度
                    cur_ += len;
                }
            }
            //返回字符串首地址
            const char* data()const{return data_;}
            //返回已写入字符长度
            int length()const{return static_cast<int>(cur_-data_);}

            //当前字符串写到那个字节
            char* current(){return cur_;}
            //返回可用空间-1
            int avail()const{return static_cast<int>(end()-cur_);}

            void add(size_t len){cur_+=len;}
            void reset(){cur_=data_;}
            //写入字符置零，但没有reset？
            void bzero(){memZero(data_, sizeof(data_));}

            const char* debugString();
            //设置构造和析构所使用的回调函数
            void setCookie(void (*cookie)()){cookie_ = cookie;}

            //返回string或StringPiece
            string toString()const{return string(data_,length());}
            StringPiece toStringPiece()const{return StringPiece(data_,length());}

        private:
            //返回尾后指针
            const char* end()const{ return data_+ sizeof(data_);}

            static void cookieStart();
            static void cookieEnd();

            void(*cookie_)(); //函数指针用于回调
            char    data_[SIZE];
            char*   cur_;
        };
    }
    class LogStream:noncopyable{
        typedef LogStream self;
    public:
        typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;
        self& operator<<(bool v){
            buffer_.append(v?"1":"0",1);
            return *this;
        }
        self& operator<<(short);
        self& operator<<(unsigned short);
        self& operator<<(int);
        self& operator<<(unsigned int);
        self& operator<<(long);
        self& operator<<(unsigned long);
        self& operator<<(long long);
        self& operator<<(unsigned long long);
        self& operator<<(const void*);
        self& operator<<(float v){
            *this<< static_cast<double>(v);
            return *this;
        }
        self& operator<<(double);
        self& operator<<(char v){
            buffer_.append(&v,1);
            return *this;
        }
        self& operator<<(const char* str){
            if(str){
                buffer_.append(str,strlen(str));
            }else{
                buffer_.append("(null)",6);
            }
            return *this;
        }
        self&operator<<(const unsigned char* str){
            return operator<<(reinterpret_cast<const char*>(str));
        }
        self&operator<<(const string& v){
            buffer_.append(v.c_str(),v.size());
            return *this;
        }
        self&operator<<(const StringPiece& v){
            buffer_.append(v.data(),v.size());
            return *this;
        }
        self&operator<<(const Buffer& v){
            *this<<v.toStringPiece();
            return *this;
        }

        void append(const char* data, int len) { buffer_.append(data, len); }

        const Buffer& buffer()const{return buffer_;}
        void resetBuffer(){buffer_.reset();}


    private:
        void staticCheck();
        template <typename T>
        void formatInteger(T);
        Buffer buffer_;
        static const int kMaxNumericSize = 32;
    };
    class Fmt{
    public:
        template<typename T>
        Fmt(const char* fmt,T val);

        const char* data()const{return buf_;}
        int length()const{return length_;}
    private:
        char buf_[32];
        int length_;
    };
    inline LogStream& operator<<(LogStream&s,const Fmt&fmt){
        s.append(fmt.data(),fmt.length());
        return s;
    }
    string formatSI(int64_t n);
    string formatIEC(int64_t n);
}

#endif //MY_TINYMUDUO_BASE_LOGSTREAM_H
