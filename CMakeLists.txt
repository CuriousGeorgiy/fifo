cmake_minimum_required(VERSION 3.16)

project(fifo C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_definitions(_GNU_SOURCE)

add_executable(files-eq files_eq.c)
add_executable(fifo-sender sender.c)
add_executable(fifo-receiver receiver.c)
