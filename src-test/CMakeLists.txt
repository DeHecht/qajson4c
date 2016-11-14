

add_executable(qajson4c-test "minimal-tests.c")
add_executable(simple-processor "simple-test.c")

target_link_libraries(qajson4c-test qajson4c)
target_link_libraries(simple-processor qajson4c)

INSTALL(TARGETS qajson4c-test simple-processor DESTINATION "tests")
