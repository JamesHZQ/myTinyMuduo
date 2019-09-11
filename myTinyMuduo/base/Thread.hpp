#ifndef MY_TINYMUDUO_BASE_THREAD_HPP
#define MY_TINYMUDUO_BASE_THREAD_HPP

#include "base/Atomic.hpp"
#include "base/CountDownLatch.hpp"

#include <functional>
#include <memory>
#include <pthread.h>
#include <string>

namespace muduo{
    class Thread:noncopyable{
    public:
        typedef std::function<void()>ThreadFunc;
        explicit Thread(ThreadFunc,const std::string &);
        ~Thread();

        void start();
        int join();

        bool started()const{return started_;}
        pid_t tid()const{return tid_;}
        const std::string& name()const{return name_;}

        static int numCreated(){return numCreated_.get();}

    private:
        void setDefaultName();
        bool               started_;
        bool               joined_;
        pthread_t          pthreadId_;
        pid_t              tid_;
        ThreadFunc         func_;
        std::string        name_;
        CountDownLatch     latch_;
        static AtomicInt32 numCreated_;
    };
}

#endif