cmake_minimum_required(VERSION 3.25)
project(futex_code LANGUAGES CXX)

add_executable(simple_futex futex_code/simple_futex.cpp)
add_executable(mutex futex_code/mutex.cpp)