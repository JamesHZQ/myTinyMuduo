cmake_minimum_required(VERSION 3.10)
project(sudoku)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(../../myTinyMuduo)
link_directories(../../myTinyMuduo/base)
link_directories(../../myTinyMuduo/net)


add_executable(sudoku1 server_basic.cpp sudoku.cpp)
target_link_libraries(sudoku1  muduo_net muduo_base pthread)

add_executable(sudoku2 server_mutiloop.cpp sudoku.cpp)
target_link_libraries(sudoku2  muduo_net muduo_base pthread)

add_executable(sudoku3 server_threadpool.cpp sudoku.cpp)
target_link_libraries(sudoku3  muduo_net muduo_base pthread)