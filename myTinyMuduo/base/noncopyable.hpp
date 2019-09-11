//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_NONCOPYABLE_HPP
#define MY_TINYMUDUO_BASE_NONCOPYABLE_HPP

//其他类可以通过继承这个类，删除自生的拷贝和赋值函数
//从而禁止该类的copy行为
namespace muduo{
    class noncopyable{
    public:
        noncopyable(const noncopyable&) = delete;
        void operator=(const noncopyable&) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };
}

#endif //MY_MUDUO_BASE_TEST_NONCOPYABLE_H
