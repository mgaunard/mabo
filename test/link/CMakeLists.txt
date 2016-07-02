add_custom_target(files)

foreach(file main test1 test2)
    add_library(${file} OBJECT ${file}.cpp)
    add_custom_command(TARGET files POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink CMakeFiles/${file}.dir/${file}.cpp.o ${CMAKE_CURRENT_BINARY_DIR}/${file}.cpp.o)
endforeach()

add_library(tests STATIC $<TARGET_OBJECTS:test1> $<TARGET_OBJECTS:test2>)
add_dependencies(tests files)