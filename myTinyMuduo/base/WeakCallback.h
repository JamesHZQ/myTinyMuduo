//
// Created by john on 4/22/19.
//

#ifndef MY_TINYMUDUO_BASE_WEAKCALLBACK_H
#define MY_TINYMUDUO_BASE_WEAKCALLBACK_H

#include <functional>
#include <memory>

namespace muduo{
    //弱回调函数对象，在对象存活的时候的调用其成员函数，在对象销毁后则不调用
    template <typename CLASS,typename... ARGS>
    class WeakCallback{
    public:
        WeakCallback(const std::weak_ptr<CLASS>& object,
                     const std::function<void(CLASS*,ARGS...)>&function)
                : object_(object),      //初始化weak_ptr指向传入的shared_ptr指向的对象并不增加引用计数（pass by ref）
                  function_(function)   //以成员函数指针以及参数组构造函数对象
        {}

        //（ARGS&&...）可以接受任意类型的实参（左值、右值）
        void operator()(ARGS&&... args)const{
            //如果类对象还存在，则可将指向其的weak_ptr提升为shared_ptr，
            // 确保调用其成员函数时，该对象不会析构
            std::shared_ptr<CLASS> ptr(object_.lock());
            if(ptr){                                                //提升成功
                //std::forward<ARGS>用于保持原始实参的类型（args绑定到右值后自己是个左值变量，
                // 为保持实参类型（右值），依然传递右值给function_，需要使用）std::forward<ARGS>
                //std::forward<ARGS>(args)...等价于std::forward<ARGS>(args1)，...，std::forward<ARGS>(argsn)
                function_(ptr.get(),std::forward<ARGS>(args)...);   //调用成员函数
            }
        }
    private:
        std::weak_ptr<CLASS> object_;
        std::function<void (CLASS*,ARGS...)> function_;
    };

    //构造并返回WeakCallback对象，该函数并不会增加shared_ptr<CLASS>的引用计数（引用传参）
    template <typename CLASS,typename... ARGS>
    WeakCallback<CLASS,ARGS...>     //可以推断出传入的成员函数（指针）的参数类型
    makeWeakCallback(const std::shared_ptr<CLASS>& object, void (CLASS::*function)(ARGS...)){
        return WeakCallback<CLASS,ARGS...>(object,function);
    }

    template <typename CLASS,typename... ARGS>
    WeakCallback<CLASS,ARGS...>
    makeWeakCallback(const std::shared_ptr<CLASS>& object, void(CLASS::*function)(ARGS...)const){
       return WeakCallback<CLASS,ARGS...>(object,function);
    }
}

#endif //MY_TINYMUDUO_BASE_WEAKCALLBACK_H
