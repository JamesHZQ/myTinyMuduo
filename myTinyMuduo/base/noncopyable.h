//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_NONCOPYABLE_H
#define MY_TINYMUDUO_BASE_NONCOPYABLE_H

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
