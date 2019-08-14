#include"net/Buffer.h"
#include"net/http/HttpContext.hpp"

using namespace muduo;
using namespace muduo::net;
/**
 * -------------------------------
 * |GET /index.html HTTP/1.1     |
 * |-----------------------------|
 * |HOST:www.baidu.com           |
 * -------------------------------
 **/
bool HttpContext::processRequestLine(const char* begin, const char* end){
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start,end,' ');
    //解析http请求保报文
    //1.获取方法（get、post、head、put、delete）
    if(space != end && request_.setMethod(start,space)){
        start = space+1;
        space = std::find(start,end,' ');
    //2.获取资源地址（'？'后的查询语句）
        if(space!=end){
            const char* question = std::find(start,space,'?');
            if(question != space){
                request_.setPath(start,question);
                request_.setQuery(question,space);
            }else{
                request_.setPath(start,space);
            }
            start = space+1;
    //3.提取末尾的8个字节，获取http协议类型（1.1or1.0）
            succeed = end-start == 8&&std::equal(start,end-1,"HTTP/1.");
            if(succeed){
                if(*(end-1)=='1'){
                    request_.setVersion(HttpRequest::kHttp11);
                }else if(*(end-1)=='0'){
                    request_.setVersion(HttpRequest::kHttp10);
                }else{
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

bool HttpContext::parseRequest(Buffer* buf,Timestamp receiveTime){
    bool ok = true;
    bool hasMore = true;
    while(hasMore){
        if(state_ == kExpectRequestLine){
            const char* crlf = buf->findCRLF();
            if(crlf){
                ok = processRequestLine(buf->peek(),crlf);
                if(ok){
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf+2);
                    state_ = kExpectHeaders;
                }else{
                    hasMore = false;
                }
            }else{
                hasMore = false;
            }
        }
        else if(state_ == kExpectHeaders){
            const char* crlf = buf->findCRLF();
            if(crlf){
                const char* colon = std::find(buf->peek(),crlf,':');
                if(colon != crlf){
                    request_.addHeader(buf->peek(),colon,crlf);
                }else{
                    state_ = kGotAll;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf+2);
            }else{
                hasMore = false;
            }
        }
        else if(state_ == kExpectBody){
            
        }
    }
    return ok;
}