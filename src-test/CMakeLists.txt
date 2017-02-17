
set(CMAKE_C_FLAGS "-std=c99")

add_executable(simple-processor "simple-test.c")

add_executable(unit_tests "unit_tests.cpp")

target_link_libraries(simple-processor qajson4c)
target_link_libraries(unit_tests qajson4c)
