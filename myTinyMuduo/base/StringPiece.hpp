//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_STRINGPIECE_HPP
#define MY_TINYMUDUO_BASE_STRINGPIECE_HPP

#include <string.h>
#include <iosfwd>
#include <string>

namespace muduo{
    //可以接受c-string和string
    //得到c-string
    class StringArg{
    public:
        StringArg(const char* str)
            : str_(str)
        {}
        StringArg(const std::string& str)
            : str_(str.c_str()) //末尾自动添加'\0'
        {}
        const char* c_str()const{
            return str_;
        }
    private:
        const char* str_;
    };
    //避免以string&为形参类型时，传入超长的const char*
    //产生大量复制的问题
    class StringPiece{
    private:
        const char* ptr_;       //字符串首地址
        int         length_;    //字符串长度
    public:
        //空串
        StringPiece():ptr_(NULL),length_(0){}
        //c-string
        StringPiece(const char* str):ptr_(str),length_(static_cast<int>(strlen(ptr_))){}
        StringPiece(const char* str,int len):ptr_(str),length_(len){}
        //string
        StringPiece(const std::string& str):ptr_(str.data()),length_(static_cast<int>(str.size())){}

        const char* data()const{return ptr_;}
        int size()const {return length_;}
        bool empty()const{return length_==0;}
        const char* begin()const{return ptr_;}
        const char* end()const{return ptr_+length_;}
        void clear(){ptr_=NULL;length_=0;}
        void set(const char* buffer,int len){ptr_=buffer;length_=len;}
        void set(const char* str){
            ptr_=str;
            length_= static_cast<int>(strlen(str));
        }
        void set(const void*buffer,int len){
            ptr_= reinterpret_cast<const char*>(buffer);
            length_=len;
        }

        char operator[](int i)const{return ptr_[i];}

        void remove_prefix(int n){
            ptr_+=n;
            length_-=n;
        }
        void remove_suffix(int n){
            length_ -= n;
        }

        bool operator==(const StringPiece& x)const{
            return ((length_==x.length_)&&(memcmp(ptr_,x.ptr_,length_)==0));
        }
        bool operator!=(const StringPiece& x)const{
            return !(*this==x);
        }
#define STRINGPIECE_BINARY_PREDICATE(cmp,auxcmp)                             \
        bool operator cmp (const StringPiece& x) const {                           \
            int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
            return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
        }
        STRINGPIECE_BINARY_PREDICATE(<,  <);
        STRINGPIECE_BINARY_PREDICATE(<=, <);
        STRINGPIECE_BINARY_PREDICATE(>=, >);
        STRINGPIECE_BINARY_PREDICATE(>,  >);
#undef STRINGPIECE_BINARY_PREDICATE                       

        //strcmp
        int compare(const StringPiece& x)const{
            int r = memcmp(ptr_,x.ptr_,length_<x.length_?length_:x.length_);
            if(r == 0){
                if(length_<x.length_) r = -1;
                else if(length_>x.length_) r += 1;
            }
            return r;
        }

        std::string as_string()const {
            return std::string(data(),size());
        }

        void CopyToString(std::string* target)const{
            target->assign(ptr_,length_);
        }

        //是否以字符串x开头
        bool starts_with(const StringPiece& x)const {
            return ((length_>=x.length_)&&(memcmp(ptr_,x.ptr_,x.length_) == 0));
        }
    };

}

#endif //MY_TINYMUDUO_BASE_STRINGPIECE_H
