
set(CMAKE_C_FLAGS "-std=c99")
set(CMAKE_CXX_FLAGS "-std=c++11")

add_executable(simple-processor "simple-test.c")

FILE (GLOB_RECURSE TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests/*.cpp )

add_executable(unit-tests "unit_tests.cpp" ${TEST_SOURCE_FILES})

target_link_libraries(simple-processor qajson4c)

target_link_libraries(unit-tests qajson4c)

target_include_directories(unit-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_test( NAME unit COMMAND unit-tests )
