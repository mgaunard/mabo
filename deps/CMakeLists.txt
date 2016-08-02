add_library(deps INTERFACE)
target_include_directories(deps INTERFACE variant/include range-v3/include)

target_link_libraries(deps INTERFACE bfd opcodes)

if(RADARE2_PATH)
    set(ENV{PKG_CONFIG_SYSROOT_DIR} ${RADARE2_PATH}/r2-static)
    set(ENV{PKG_CONFIG_PATH} ${RADARE2_PATH}/r2-static/usr/lib/pkgconfig)
    execute_process(COMMAND pkg-config --libs r_core OUTPUT_VARIABLE LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND pkg-config --cflags-only-I r_core OUTPUT_VARIABLE INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)

    string(REPLACE " " ";" LIBS "${LIBS}")
    string(REGEX REPLACE " *-I" ";" INCLUDE_DIRS "${INCLUDE_DIRS}")
    string(REGEX REPLACE "^;" "" INCLUDE_DIRS "${INCLUDE_DIRS}")

    target_include_directories(deps INTERFACE ${INCLUDE_DIRS})
    target_compile_definitions(deps INTERFACE -DMABO_WITH_RADARE2)

    file(GLOB_RECURSE LIBS ${RADARE2_PATH}/*.a)
    target_link_libraries(deps INTERFACE -L$ENV{PKG_CONFIG_SYSROOT_DIR}/usr/lib ${LIBS} -lr_crypto -lutil -ldl -pthread)
endif()
