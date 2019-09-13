#ifndef MY_TINYMUDUO_ASYNCLOGGING_HPP
#define MY_TINYMUDUO_ASYNCLOGGING_HPP

#include"base/CountDownLatch.hpp"
#include"base/Mutex.hpp"
#include"base/Thread.hpp"
#include"base/Logging.hpp"
#include"base/LogStream.hpp"

#include<atomic>
#include<string>
#include<vector>

namespace muduo
{
    class AsyncLogging:noncopyable
    {
    private:
        void threadFunc();
        typedef detail::FixedBuffer<detail::kLargeBuffer> Buffer;
        typedef std::vector<std::unique_ptr<Buffer>> BufferPtrVector;
        typedef BufferPtrVector::value_type BufferPtr;
        
        const std::string   basename_;
        std::atomic<bool>   running_;
        const int           flushInterval_;        
        mutable MutexLock   mutex_;
        Condition           cond_;        
        CountDownLatch      latch_;  
        BufferPtr           currentBuffer_;
        BufferPtr           nextBuffer_;
        BufferPtrVector     buffers_;
        Thread              thread_;
    public:
        AsyncLogging(const std::string& basename,int flushInteral = 3);
        ~AsyncLogging()
        {
            if(running_)
            {
                stop();
            }
        }
        void append(const char* logline,int len);
        void start()
        {
            running_ = true;
            thread_.start();
            latch_.wait();
        }
        void stop()
        {
            running_ = false;
            cond_.notify();
            thread_.join();
        }
    };
}

#endif