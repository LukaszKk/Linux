cmake_minimum_required(VERSION 3.13)
project(untitled1 C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

add_executable(untitled1 main.c)

target_link_libraries(untitled1 -lrt)