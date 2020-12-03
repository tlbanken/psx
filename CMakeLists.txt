cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(psx VERSION 0.01)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")


add_executable(psx 
    src/psx.cpp
)

target_include_directories(psx PRIVATE $(PROJECT_SOURCE_DIR)/include)

