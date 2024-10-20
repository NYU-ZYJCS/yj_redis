cmake_minimum_required(VERSION 3.0)

project(redis_learning_03)

set(CMAKE_CXX_STANDARD 20)

add_executable(03_server 03_server.cpp)
add_executable(03_client 03_client.cpp)
