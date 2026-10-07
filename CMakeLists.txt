cmake_minimum_required(VERSION 3.16)
project(labTwo_2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

include_directories(headers)

add_executable(labTwo_2 src/main.cpp src/Tests.cpp)