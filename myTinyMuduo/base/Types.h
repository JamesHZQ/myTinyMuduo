//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_TYPES_H
#define MY_TINYMUDUO_BASE_TYPES_H

#include <stdint.h>
#include <string.h>
#include <string>
#ifdef NDEBUG
#include <assert.h>
#endif

namespace muduo{
    using std::string;
    inline void memZero(void *p,size_t n){
        //内存置零
        memset(p,0,n);
    }

    template <typename To,typename  From>
    inline To implicit_cast(From const &f){
        return f;
    }

    //？？就是static_cast
    template <typename To,typename From>
    inline To down_cast(From *f){
        if(false){
            implicit_cast<From*,To>(0);
        }
#if !defined(NDEBUG)&&!defined(GOOLGLE_PROTOBUF_NO_RTTI)
        assert(f==NULL|| dynamic_cast<To>(f)!=NULL);
#endif
        return static_cast<To>(f);
    }
}

#endif //MY_MUDUO_BASE_TEST_TYPES_H
