cmake_minimum_required(VERSION 3.10)
project(filetransfer)

set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(../../myTinyMuduo)
link_directories(../../myTinyMuduo/base)
link_directories(../../myTinyMuduo/net)


add_executable(filetransfer download.cpp)

target_link_libraries(filetransfer  muduo_net muduo_base pthread)
