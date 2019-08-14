cmake_minimum_required(VERSION 3.10)
project(my_tinymuduo_net)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_CXX_STANDARD 14)
include_directories(../)
link_directories(../base)
set(net_SRCS
        EventLoop.cpp
        Channel.cpp
        Socket.cpp
        SocketsOps.cpp
        Poller.cpp
        Timer.cpp
        TimerQueue.cpp
        poller/PollPoller.cpp
        poller/DefaultPoller.cpp
        InetAddress.cpp
        Acceptor.cpp
        Connector.cpp
        Buffer.cpp
        TcpConnection.cpp
        EventLoopThread.cpp
        EventLoopThreadPool.cpp
        TcpServer.cpp
        TcpClient.cpp
        poller/EPollPoller.cpp
        http/HttpContext.cpp
        http/HttpRespose.cpp
        http/HttpServer.cpp
        )
add_library(muduo_net ${net_SRCS})

target_link_libraries(muduo_net muduo_base)
