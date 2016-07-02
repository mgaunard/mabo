cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE Debug)
add_definitions(-Wall -Wextra -fdiagnostics-color=always -std=c++14)

add_subdirectory(deps)

add_library(mabo INTERFACE)
target_include_directories(mabo INTERFACE include)
target_link_libraries(mabo INTERFACE deps)

add_subdirectory(test)

add_executable(mabo_main main.cpp)
set_property(TARGET mabo_main PROPERTY OUTPUT_NAME mabo)
target_link_libraries(mabo_main mabo)
