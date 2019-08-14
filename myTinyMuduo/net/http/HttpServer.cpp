#include"net/http/HttpServer.hpp"

#include"base/Logging.h"
#include"net/http/HttpContext.hpp"
#include"net/http/HttpRequest.hpp"
#include"net/http/HttpRespose.hpp"

using namespace muduo;
using namespace muduo::net;

namespace muduo{
    namespace net{
        namespace detail{
            void  defaultHttpCallback(const HttpRequest&,HttpResponse* resp){
                resp->setStatusCode(HttpResponse::k404NotFound);
                resp->setStatusMessage("Not Found");
                resp->setCloseConnection(true);
            }
        }
    }
}

HttpServer::HttpServer(EventLoop* loop, const InetAddress& listenAddr,const string& name,
                        TcpServer::Option option)
    : server_(loop,listenAddr,name,option),
      httpCallback_(detail::defaultHttpCallback)
{
    server_.setConnectionCallback(std::bind(&HttpServer::onConnection,this,_1));
    server_.setMessageCallback(std::bind(&HttpServer::onMessage,this,_1,_2,_3));
}
void HttpServer::start(){
    LOG_WARN<<"HttpServer["<<server_.name()
        <<"] starts listenning on "<<server_.ipPort();
    server_.start();
}
void HttpServer::onConnection(const TcpConnectionPtr& conn){
    if(conn->connected()){
        conn->setContext(HttpContext());
    }
}
void HttpServer::onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp receiveTime){
    HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContex());
    if(!context->parseRequest(buf,receiveTime)){
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }
    if(context->gotAll()){
        //解析http请求成功，获取结果HttpRequest对象
        onRequest(conn,context->request());
        //重置conn的context的HttpRequest对象
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req){
    const string& connection = req.getHeader("Connection");
    //长连接or短连接
    bool close = connection == "close"||
        (req.getVersion()==HttpRequest::kHttp10 && connection != "Keep-Alive");
    HttpResponse response(close);
    //httpCallback_设置HttpResponse对象
    httpCallback_(req,&response);
    Buffer buf;
    //生成响应报文
    response.appendToBuffer(&buf);
    conn->send(&buf);
    //若是短连接，发送完响应报文，服务端shutdown发送一个FIN（客户端会读到0字节，通常客户端会就此断开连接）
    if(response.closeConnection()){
        conn->shutdown();
    }
}