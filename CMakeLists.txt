cmake_minimum_required(VERSION 3.24)
project(MyJson_Parser)
set(CMAKE_CXX_STANDARD 20)
#[[头文件目录]]
include_directories(include)
#[[测试文件]]
add_executable(${PROJECT_NAME} src/main.cpp)
