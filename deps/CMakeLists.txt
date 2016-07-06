add_library(deps INTERFACE)
target_include_directories(deps INTERFACE variant/include range-v3/include)

target_link_libraries(deps INTERFACE bfd opcodes)

set(ENV{PKG_CONFIG_SYSROOT_DIR} /home/mgaunard/dev/radare2/r2-static)
set(ENV{PKG_CONFIG_PATH} /home/mgaunard/dev/radare2/r2-static/usr/lib/pkgconfig)
execute_process(COMMAND pkg-config --libs r_core OUTPUT_VARIABLE LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND pkg-config --cflags-only-I r_core OUTPUT_VARIABLE INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REPLACE " " ";" LIBS "${LIBS}")
string(REGEX REPLACE " *-I" ";" INCLUDE_DIRS "${INCLUDE_DIRS}")
string(REGEX REPLACE "^;" "" INCLUDE_DIRS "${INCLUDE_DIRS}")

target_include_directories(deps INTERFACE ${INCLUDE_DIRS})

file(GLOB_RECURSE LIBS /home/mgaunard/dev/radare2/*.a)
target_link_libraries(deps INTERFACE -L$ENV{PKG_CONFIG_SYSROOT_DIR}/usr/lib ${LIBS} -lr_crypto -lutil -ldl -pthread)