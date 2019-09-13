#include"base/AsyncLogging.hpp"
#include"base/LogFile.hpp"
#include"Timestamp.hpp"

#include<stdio.h>

using namespace muduo;
using namespace std;

AsyncLogging::AsyncLogging(const string& basename,int flushInterval)
    :   basename_(basename),
        running_(false),
        flushInterval_(flushInterval),
        mutex_(),
        cond_(mutex_),
        latch_(1),
        currentBuffer_(new Buffer()),
        nextBuffer_(new Buffer()),
        buffers_(),
        thread_(bind(&AsyncLogging::threadFunc,this),"Logging")
{
    buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len)
{
    MutexLockGuard lock(mutex_);
    if(currentBuffer_->avail()>len)
    {
        currentBuffer_->append(logline,len);
    }
    else
    {
        buffers_.push_back(std::move(currentBuffer_));
        if(nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            currentBuffer_.reset(new Buffer());
        }
        currentBuffer_->append(logline,len);
        cond_.notify();
    }
}

void AsyncLogging::threadFunc()
{
    assert(running_==true);
    latch_.countDown();
    LogFile output(basename_);
    BufferPtr newBuffer1(new Buffer());
    BufferPtr newBuffer2(new Buffer());
    BufferPtrVector buffersToWrite;
    buffersToWrite.reserve(16);
    while(running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());
        {
            MutexLockGuard lock(mutex_);
            if(buffers_.empty())
            {
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            swap(buffersToWrite,buffers_);
            //说明nexBuffer_若为空,说明前端将currentBuffer_“移动“到buffers_
            //所以前端的buffers_至少有一个”BufferPtr“，再加上后端在交换之前，会将
            //此时的currentBuffer_（可能里面没有内容）“移动”到buffers_，所以
            //可以保证在newBuffer2被移动之后，自己变为“空指针”时，buffersToWrite
            //里至少有两个“BufferPtr”
            if(!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());

        if(buffersToWrite.size()>25)
        {
            char buf[256];
            snprintf(buf,sizeof(buf),"Dropped log messages at %s,%zd larger buffers\n",
                        Timestamp::now().toFormattedString().c_str(),
                        buffersToWrite.size()-2);
            fputs(buf,stderr);
            output.append(buf,static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin()+2,buffersToWrite.end());   
        }
        for(const auto& buffer:buffersToWrite)
        {
            output.append(buffer->data(),buffer->length());
        }
        //清理空间
        //一定留下一个buffer的空间给newBuffer1
        //可能留下一个buffer的空间个newBuffer2
        if(buffersToWrite.size()>2)
        {
            buffersToWrite.resize(2);
        }
        if(!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }
        if(!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }
        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}