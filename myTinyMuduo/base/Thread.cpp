#include "base/Thread.h"
#include "base/CurrentThread.h"
#include "base/Exception.h"
#include "base/Logging.h"

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo{
    namespace detail{
        pid_t gettid(){
            return static_cast<pid_t >(::syscall(SYS_gettid));
        }
        void afterFork(){
            muduo::CurrentThread::t_cachedTid = 0;
            muduo::CurrentThread::t_threadName = "main";
            CurrentThread::tid();   //重新设置t_cachedTid
        }
        class ThreadNameInitializer{
        public:
            ThreadNameInitializer(){
                muduo::CurrentThread::t_threadName = "main";
                CurrentThread::tid();
                //注册回调函数
                //在当前线程使用fork后，会调用afterFork
                //应该是用不到，不需要fork
                pthread_atfork(NULL,NULL,&afterFork);CurrentThread::tid();
            }
        };
        //程序一开始就要执行muduo::CurrentThread::t_threadName = "main";CurrentThread::tid();
        ThreadNameInitializer init;


        struct ThreadData{
            typedef muduo::Thread::ThreadFunc ThreadFunc;
            ThreadFunc      func_;
            string          name_;
            pid_t*          tid_;
            CountDownLatch* latch_;

            ThreadData(ThreadFunc func,const string& name,pid_t* tid,CountDownLatch* latch)
                : func_(std::move(func)),
                  name_(name),
                  tid_(tid),
                  latch_(latch)
            {}

            void runInThread(){
                *tid_ = muduo::CurrentThread::tid();
                tid_ = NULL;
                latch_->countDown();
                latch_ = NULL;
                muduo::CurrentThread::t_threadName = name_.empty()?"muduoThread":name_.c_str();
                ::prctl(PR_SET_NAME,muduo::CurrentThread::t_threadName);
                try{
                    func_();
                    muduo::CurrentThread::t_threadName = "finished";
                }catch (const Exception& ex){
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr,"exception caught in Thread %s\n",name_.c_str());
                    fprintf(stderr,"reason: %s\n",ex.what());
                    abort();
                }catch (const std::exception& ex){
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr,"reason: %s\n",ex.what());
                    abort();
                }catch(...){
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr,"unknown exception caught in Thread %s\n",name_.c_str());
                    throw;
                }
            }
        };

        void* startThread(void* obj){
            ThreadData* data = static_cast<ThreadData*>(obj);
            data->runInThread();
            delete data;
            return NULL;
        }
    }

    void CurrentThread::cacheTid() {
        if(t_cachedTid==0){
            //调用gettid()，将得到的本线程的tid赋给线程特定数据t_cachedTid
            t_cachedTid = detail::gettid();
            //格式化tid，结果存到t_tidString，长度t_tidStringLength
            t_tidStringLength = snprintf(t_tidString,sizeof(t_tidString),"%5d",t_cachedTid);
        }
    }

    bool CurrentThread::isMainThread() {
        //当前线程的tid如果和进程的pid相等就说明当前线程为主线程
        return tid() == ::getpid();
    }

    void CurrentThread::sleepUsec(int64_t usec) {
        //让线程阻塞一段时间，精确到纳秒
        struct timespec ts = {0,0};
        ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
        ::nanosleep(&ts, NULL);
    }

    //初始化静态成员
    AtomicInt32 Thread::numCreated_;



    Thread::Thread(ThreadFunc func, const string &name)
        : started_(false),
          joined_(false),
          pthreadId_{0},
          tid_(0),
          func_(std::move(func)),
          name_(name),
          latch_(1)
    {
        setDefaultName();
    }
    Thread::~Thread() {
        if(started_ && !joined_){
            //如果开启的进程还没有结束（还没join）
            //将开起的线程分离
            pthread_detach(pthreadId_);
        }
    }

    //（原子的）递增静态成员变量保证每个线程的name_都是唯一的
    void Thread::setDefaultName() {
        int num = numCreated_.incrementAndGet();
        if(name_.empty()){
            char buf[32];
            snprintf(buf,sizeof(buf),"Thread%d",num);
            name_ = buf;
        }
    }

    void Thread::start() {
        assert(!started_);
        started_ = true;
        //ThreadData匿名对象在堆中，注意清理
        detail::ThreadData* data = new detail::ThreadData(func_,name_,&tid_,&latch_);
        if(pthread_create(&pthreadId_,NULL,&detail::startThread,data))
        {
            //pthread_create返回值不等于0表示线程创建失败
            started_ = false;
            //调用detail::startThread失败，需要在这里释放detail::ThreadData占有的空间
            delete data;
            LOG_SYSFATAL<<"Failed in pthread_create";
        }else{
            //确保tid已正确设置
            latch_.wait();
            assert(tid_>0);
        }
    }
    int Thread::join(){
        assert(started_);
        assert(!joined_);
        joined_ = true;
        return pthread_join(pthreadId_,NULL);
    }

}