//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_CURRENTTHREAD_HPP
#define MY_TINYMUDUO_BASE_CURRENTTHREAD_HPP
#include<stdint.h>

namespace muduo
{
    namespace CurrentThread
    {
        //线程特定数据
        extern __thread int         t_cachedTid;
        extern __thread char        t_tidString[32];
        extern __thread int         t_tidStringLength;
        extern __thread const char* t_threadName;
        //将本线程tid存到t_cachedTid
        void cacheTid();

        inline int tid(){
            if (__builtin_expect(t_cachedTid == 0, 0)){
                cacheTid();
            }
            return t_cachedTid;
        }
        // for logging
        inline const char* tidString() {
            return t_tidString;
        }
        // for logging
        inline int tidStringLength() {
            return t_tidStringLength;
        }

        inline const char* name(){
            return t_threadName;
        }

        bool isMainThread();

        void sleepUsec(int64_t usec);

        //string stackTrace(bool demangle);


    }
}




#endif //MY_MUDUO_BASE_TEST_CURRENTTHREAD_H
