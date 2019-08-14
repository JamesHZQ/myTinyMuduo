#ifndef MUDUO_NET_HTTP_HTTPCONTEXT_H
#define MUDUO_NET_HTTP_HTTPCONTEXT_H

#include"base/copyable.h"
#include"net/http/HttpRequest.hpp"

namespace muduo{
    namespace net{
        class Buffer;
        class HttpContext:public copyable{
            public:
                enum HttpRequestPareseState{
                    kExpectRequestLine,
                    kExpectHeaders,
                    kExpectBody,
                    kGotAll,
                };
                HttpContext():state_(kExpectRequestLine){}
                bool parseRequest(Buffer* buf,Timestamp receiveTime);
                bool gotAll()const{
                    return state_ == kGotAll;
                }
                void reset(){
                    state_ = kExpectRequestLine;
                    HttpRequest dummy;
                    request_.swap(dummy);
                }
                const HttpRequest& request()const{
                    return request_;
                }
                HttpRequest& request(){
                    return request_;
                }
            private:
                bool processRequestLine(const char* begin, const char* end);
                HttpRequestPareseState state_;
                HttpRequest request_;
        };

    }// namespace net

    
} // namespace muduo




#endif  // MUDUO_NET_HTTP_HTTPCONTEXT_H
