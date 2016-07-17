# dummy binaries for testing
add_custom_target(files)

foreach(file main test1 test2)
    add_library(${file} OBJECT ${file}.cpp)
    add_custom_command(TARGET files POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink CMakeFiles/${file}.dir/${file}.cpp.o ${CMAKE_CURRENT_BINARY_DIR}/${file}.cpp.o)
endforeach()

add_library(tests STATIC $<TARGET_OBJECTS:test1> $<TARGET_OBJECTS:test2>)
add_dependencies(tests files)

add_executable(test_exe $<TARGET_OBJECTS:main>)
target_link_libraries(test_exe tests)

# shared rebuilt for PIC
add_library(tests_shared SHARED test1.cpp test2.cpp)
add_dependencies(tests_shared files)

add_library(test1_shared SHARED test1.cpp)
add_dependencies(test1_shared files)

add_library(test2_shared SHARED test2.cpp)
add_dependencies(test2_shared files)
set_property(TARGET test2_shared PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/2)

add_executable(test_exe_shared main.cpp)
target_link_libraries(test_exe_shared test1_shared)

add_executable(test_exe_shared2 main.cpp main2.cpp)
target_link_libraries(test_exe_shared2 test1_shared test2_shared)

# actual test
add_executable(binary binary.cpp)
target_link_libraries(binary mabo)
add_test(binary binary)