cmake_minimum_required(VERSION 3.21)
project(lbr)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES main.cpp a_star.cpp a_star.hpp)

add_executable(lbr ${SOURCE_FILES})