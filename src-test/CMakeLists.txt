
set(CMAKE_C_FLAGS "-std=c99")
set(CMAKE_CXX_FLAGS "-std=c++11")

add_executable(simple-processor "simple-test.c")

add_executable(unit-tests "unit_tests.cpp")

target_link_libraries(simple-processor qajson4c)
target_link_libraries(unit-tests qajson4c)

add_test( NAME unit COMMAND unit-tests )
