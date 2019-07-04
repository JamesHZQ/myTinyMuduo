cmake_minimum_required(VERSION 3.10)
project(time)

set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(../../myTinyMuduo)
link_directories(../../myTinyMuduo/base)
link_directories(../../myTinyMuduo/net)


add_executable(daytime daytime.cpp main.cpp)
target_link_libraries(daytime muduo_net muduo_base pthread)

add_executable(daytimeclient timeclient.cpp)
target_link_libraries(daytimeclient muduo_net muduo_base pthread)