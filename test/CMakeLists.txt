find_package(Threads REQUIRED)

include(ExternalProject)
ExternalProject_Add(gtest_gmock
                    GIT_REPOSITORY https://github.com/google/googletest.git
                    PATCH_COMMAND patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/gtest.patch
                    CMAKE_ARGS -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
                    INSTALL_COMMAND ""
                    )

ExternalProject_Get_Property(gtest_gmock source_dir binary_dir)

add_library(gtest STATIC IMPORTED)
add_dependencies(gtest gtest_gmock)
set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${binary_dir}/googlemock/gtest/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
file(MAKE_DIRECTORY ${source_dir}/googletest/include)
set_property(TARGET gtest PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${source_dir}/googletest/include)
#include_directories(${source_dir}/googletest/include)

add_library(gtest_main STATIC IMPORTED)
add_dependencies(gtest_main gtest_gmock)
set_property(TARGET gtest_main PROPERTY IMPORTED_LOCATION ${binary_dir}/googlemock/gtest/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX})
set_property(TARGET gtest_main PROPERTY INTERFACE_LINK_LIBRARIES gtest)

add_library(gmock STATIC IMPORTED)
add_dependencies(gmock gtest_gmock)
set_property(TARGET gmock PROPERTY IMPORTED_LOCATION ${binary_dir}/googlemock/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX})
file(MAKE_DIRECTORY ${source_dir}/googlemock/include)
set_property(TARGET gmock PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${source_dir}/googlemock/include)
#include_directories(${source_dir}/googlemock/include)
set_property(TARGET gmock PROPERTY INTERFACE_LINK_LIBRARIES gtest ${CMAKE_THREAD_LIBS_INIT})

link_libraries(gtest_main gmock gtest)
include_directories(.)

add_subdirectory(binary)
