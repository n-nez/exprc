cmake_minimum_required(VERSION 3.8) # for c++17

project(exprc)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror")

add_executable(exprc
    src/main.cpp
    src/dfg.cpp
)

target_include_directories(exprc
    PRIVATE include
)
target_link_libraries(exprc
    fmt
)
