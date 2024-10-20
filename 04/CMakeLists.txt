cmake_minimum_required(VERSION 3.0)

project(redis_learning_04)

set(CMAKE_CXX_STANDARD 20)

add_executable(04_server 04_server.cpp)
add_executable(04_client 04_client.cpp)
