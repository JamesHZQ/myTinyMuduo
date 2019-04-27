//
// Created by john on 4/22/19.
//

#ifndef MY_TINYMUDUO_BASE_WEAKCALLBACK_H
#define MY_TINYMUDUO_BASE_WEAKCALLBACK_H

#include <functional>
#include <memory>

namespace muduo{
    template <typename CLASS,typename... ARGS>
    class WeakCallback{
    public:
        WeakCallback(const std::weak_ptr<CLASS>& object,                    //存在类型转换指向对象的shared_ptr 》 weak_ptr
                     const std::function<void(CLASS*,ARGS...)>&function)    //存在类型转换成员函数指针转换为function对象
                : object_(object),      //指向回调对象的weak_ptr,不会增加shared_ptr的引用计数，从而不会延长对象的生命周期
                  function_(function)   //成员函数的function原型的第一个参数为类对象指针（或对象）后面为成员函数自己的参数
        {}

        void operator()(ARGS&&... args)const{
            std::shared_ptr<CLASS> ptr(object_.lock());             //如果类对象还存在，则可将指向其的weak_ptr提升为shared_ptr
            if(ptr){                                                //提升成功
                function_(ptr.get(),std::forward<ARGS>(args)...);    //调用成员函数
            }
        }
    private:
        std::weak_ptr<CLASS> object_;
        std::function<void (CLASS*,ARGS...)> function_;
    };

    //构造弱回调对象，在对象存活的时候的调用其成员函数，在对象销毁后则不调用
    template <typename CLASS,typename... ARGS>
    WeakCallback<CLASS,ARGS...>makeWeakCallback(const std::shared_ptr<CLASS>& object,
                                                void (CLASS::*function)(ARGS...))
    {
        return WeakCallback<CLASS,ARGS...>(object,function);
    }

    template <typename CLASS,typename... ARGS>
    WeakCallback<CLASS,ARGS...>makeWeakCallback(const std::shared_ptr<CLASS>& object,
                                                void(CLASS::*function)(ARGS...)const)
    {
       return WeakCallback<CLASS,ARGS...>(object,function);
    }
}

#endif //MY_TINYMUDUO_BASE_WEAKCALLBACK_H
