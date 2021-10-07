cmake_minimum_required(VERSION 3.20)

project(fifo C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(fifo-sender sender.c)
add_executable(fifo-receiver receiver.c)
