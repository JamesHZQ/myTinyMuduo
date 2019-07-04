cmake_minimum_required(VERSION 3.10)
project(finger)

set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(../../myTinyMuduo)
link_directories(../../myTinyMuduo/base)
link_directories(../../myTinyMuduo/net)


add_executable(finger1 finger1.cpp )
target_link_libraries(finger1  muduo_net muduo_base pthread)

add_executable(finger2 finger2.cpp)
target_link_libraries(finger2  muduo_net muduo_base pthread)

add_executable(finger3 finger3.cpp)
target_link_libraries(finger3  muduo_net muduo_base pthread)

add_executable(finger4 finger4.cpp)
target_link_libraries(finger4  muduo_net muduo_base pthread)

add_executable(finger5 finger5.cpp)
target_link_libraries(finger5  muduo_net muduo_base pthread)

add_executable(finger6 finger6.cpp)
target_link_libraries(finger6  muduo_net muduo_base pthread)