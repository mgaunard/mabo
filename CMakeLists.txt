cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE Debug)
add_definitions(-Wall -Wextra -fdiagnostics-color=always -std=c++14)

add_subdirectory(deps)
link_libraries(deps)

add_executable(mabo main.cpp)