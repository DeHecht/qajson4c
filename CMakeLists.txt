
cmake_minimum_required(VERSION 2.8.12)

project(libqajson4c C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -pedantic-errors -Wall -Wextra")

add_subdirectory(src)
add_subdirectory(src-test)
