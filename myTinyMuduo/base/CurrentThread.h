//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_CURRENTTHREAD_H
#define MY_TINYMUDUO_BASE_CURRENTTHREAD_H

#include "base/Types.h"

namespace muduo
{
    namespace CurrentThread
    {
        // internal
        extern __thread int         t_cachedTid;
        extern __thread char        t_tidString[32];
        extern __thread int         t_tidStringLength;
        extern __thread const char* t_threadName;
        //将本线程tid存到t_cachedTid
        void cacheTid();

        inline int tid()
        {
            if (__builtin_expect(t_cachedTid == 0, 0))
            {
                cacheTid();
            }
            return t_cachedTid;
        }

        inline const char* tidString() // for logging
        {
            return t_tidString;
        }

        inline int tidStringLength() // for logging
        {
            return t_tidStringLength;
        }

        inline const char* name()
        {
            return t_threadName;
        }

        bool isMainThread();

        void sleepUsec(int64_t usec);

        //string stackTrace(bool demangle);


    }
}




#endif //MY_MUDUO_BASE_TEST_CURRENTTHREAD_H
