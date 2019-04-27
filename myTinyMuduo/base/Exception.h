//
// Created by john on 4/15/19.
//

#ifndef MY_TINYMUDUO_BASE_EXCEPTION_H
#define MY_TINYMUDUO_BASE_EXCEPTION_H

#include "base/Types.h"
#include <exception>

namespace muduo{
class Exception:public std::exception{
public:
    Exception(string);
    ~Exception()noexcept override = default;

    //重载what函数返回错误信息
    const char* what()const noexcept override {
        return message_.c_str();
    }

private:
    string message_;
    //string stack_;
};
}


#endif //MY_TINYMUDUO_BASE_EXCEPTION_H
