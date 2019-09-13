cmake_minimum_required(VERSION 3.10)
project(echo)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_CXX_STANDARD 17)

INCLUDE_DIRECTORIES(../../myTinyMuduo)
link_directories(../../myTinyMuduo/base)
link_directories(../../myTinyMuduo/net)


add_executable(echo main.cpp echo.h echo.cpp)

target_link_libraries(echo  muduo_net muduo_base pthread)
