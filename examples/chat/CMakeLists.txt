cmake_minimum_required(VERSION 3.10)
project(chat)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(../../myTinyMuduo)
link_directories(../../myTinyMuduo/base)
link_directories(../../myTinyMuduo/net)


add_executable(chatclient chatClient.cpp )
target_link_libraries(chatclient muduo_net muduo_base pthread)

add_executable(chatserver chatServer.cpp)
target_link_libraries(chatserver muduo_net muduo_base pthread)

add_executable(chatserver1 chatServerwithThread.cpp)
target_link_libraries(chatserver1 muduo_net muduo_base pthread)