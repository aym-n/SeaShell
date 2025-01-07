cmake_minimum_required(VERSION 3.10)
project(seashell)

set(CMAKE_C_STANDARD 99)

include_directories(src)

set(SOURCES
    src/main.c
    src/utils.c
    src/builtins.c
    src/shell.c
)

add_executable(seashell ${SOURCES})
