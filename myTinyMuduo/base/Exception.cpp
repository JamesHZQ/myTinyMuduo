//
// Created by john on 4/15/19.
//

#include "base/Exception.h"
#include "base/CurrentThread.h"

namespace muduo{
    Exception::Exception(std::string msg)
        : message_(std::move(msg))
          //stack_(CurrentThread::stackTrace(/*demangle=*/false))
    {}
}