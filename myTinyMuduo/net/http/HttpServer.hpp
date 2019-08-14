#ifndef MUDUO_NET_HTTPSERVER_H
#define MUDUO_NET_HTTPSERVER_H

#include"net/TcpServer.h"
namespace muduo{
    namespace net{
        class HttpRequest;
        class HttpResponse;

        class HttpServer : noncopyable{
            public:
                typedef std::function<void(const HttpRequest&,HttpResponse*)> HttpCallback;
                HttpServer(EventLoop* loop,const InetAddress& listenAddr, const string& name, 
                            TcpServer::Option option=TcpServer::kNoReusePort);
                EventLoop* getLoop()const{
                    return server_.getLoop();
                }
                void setHttpCallback(const HttpCallback& cb){
                    httpCallback_ = cb;
                }
                void setThreadNum(int numThreads){
                    server_.setThreadNum(numThreads);
                }
                void start();

            private:
                void onConnection(const TcpConnectionPtr& conn);
                void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime);
                void onRequest(const TcpConnectionPtr&, const HttpRequest&);

                TcpServer server_;
                HttpCallback httpCallback_;
        };
    }
}




#endif //MUDUO_NET_HTTPSERVER_H