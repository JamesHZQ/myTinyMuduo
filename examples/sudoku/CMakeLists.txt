cmake_minimum_required(VERSION 3.10)
project(sudoku)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(../../myTinyMuduo)
link_directories(../../myTinyMuduo/base)
link_directories(../../myTinyMuduo/net)


add_executable(sudoku server_basic.cpp sudoku.cpp)

target_link_libraries(sudoku  muduo_net muduo_base pthread)
