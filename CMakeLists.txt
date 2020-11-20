cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 17)

project(PSX VERSION 0.01)

add_executable(psx 
    src/psx.cpp
)


target_include_directories(psx PRIVATE $(PROJECT_SOURCE_DIR)/include)
