#include"sudoku.h"
#include"base/Atomic.h"
#include"base/Logging.h"
#include"base/Thread.h"
#include"base/ThreadPool.h"
#include"net/EventLoop.h"
#include"net/InetAddress.h"
#include"net/TcpServer.h"

#include<utility>
#include<stdio.h>
#include<unistd.h>

using namespace muduo;
using namespace muduo::net;


class SudokuServer{
    public:
    SudokuServer(EventLoop* loop,const InetAddress& listenAddr,int numThreads)
        : server_(loop,listenAddr,"SudokuServer"),
          startTime_(Timestamp::now()),
          numThreads_(numThreads)
    {
        server_.setConnectionCallback(std::bind(&SudokuServer::onConnection,this,_1));
        server_.setMessageCallback(std::bind(&SudokuServer::onMessage,this,_1,_2,_3));
        server_.setThreadNum(numThreads);
    }
    void start(){
        LOG_INFO<<"starting "<<numThreads_<<" threads.";
        threadpool_.start(numThreads_);
        server_.start();
    }

    private:
        void onConnection(const TcpConnectionPtr& conn){
            LOG_INFO<<conn->peerAddress().toIpPort()<<" -> "
                     <<conn->localAddress().toIpPort()<<" is "
                     <<(conn->connected()?"UP":"DOWN");
        }
        void onMessage(const TcpConnectionPtr& conn, Buffer* buf,Timestamp){
            LOG_INFO<<conn->name();
            size_t len = buf->readableBytes();
            //当缓冲区长度大于kCells+2，提取读缓冲区的数据
            //（可能一次受到多个请求，循环处理）
            //如果收到非法请求服务端会断开连接
            while(len>=kCells+1){
                const char* eof = buf->findEOL();
                if(eof){
                    string request(buf->peek(),eof);
                    buf->retrieveUntil(eof+1);
                    len = buf->readableBytes();
                    if(!processRequest(conn,request)){
                        conn->send("Bad Request!\r\n");
                        conn->shutdown();
                        break;
                    }
                }else if(len>100){
                    conn->send("Id too long!\r\n");
                    conn->shutdown();
                    break;
                }else{
                    break;
                }
            }
        }
        bool processRequest(const TcpConnectionPtr& conn,const string &request){
            string id;
            string puzzle;
            bool goodRequest = true;
            //提取ID：（矩阵）
            string::const_iterator colon = find(request.begin(),request.end(),':');
            if(colon!=request.end()){
                id.assign(request.begin(),colon);
                puzzle.assign(colon+1,request.end());
            }else{
                puzzle = request;
            }
            if(puzzle.size() == implicit_cast<size_t>(kCells)){
                threadpool_.run(std::bind(&solve,conn,puzzle,id));
            }else{
                goodRequest = false;
            }
            return goodRequest;
        }

        static void solve(const TcpConnectionPtr& conn,const string& puzzle,const string& id){
            LOG_INFO<<conn->name();
            string result=solveSudoku(puzzle);
            if(id.empty()){
                conn->send(result+"\r\n");
            }else{
                conn->send(id+":"+result+"\r\n");
            }
        }

        TcpServer server_;
        Timestamp startTime_;
        int numThreads_; //I/0线程数
        ThreadPool threadpool_;//线程池
        

};

int main(int argc,char* argv[]){
    LOG_INFO<<"pid = "<<getpid()<<", tid = "<<CurrentThread::tid();
    int numThreads = 0;
    if(argc>1){
        numThreads = atoi(argv[1]);
    }
    EventLoop loop;
    InetAddress listenAddr(9981);
    SudokuServer server(&loop,listenAddr,numThreads);
    server.start();
    loop.loop();
}