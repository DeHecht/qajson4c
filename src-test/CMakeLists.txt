

add_executable(qajson4c-minimal "minimal-tests.c")
add_executable(simple-test "simple-test.c")

target_link_libraries(qajson4c-minimal qajson4c)
target_link_libraries(simple-test qajson4c)

INSTALL(TARGETS qajson4c-minimal simple-test DESTINATION "tests")
