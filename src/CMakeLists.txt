
FILE (GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c )

add_library(qajson4c-obj OBJECT ${SOURCE_FILES})

add_library(qajson4c STATIC $<TARGET_OBJECTS:qajson4c-obj> )
add_library(qajson4c-shared SHARED $<TARGET_OBJECTS:qajson4c-obj> )

target_include_directories(qajson4c-obj PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(qajson4c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(qajson4c-obj PROPERTIES POSITION_INDEPENDENT_CODE True) 
set_target_properties(qajson4c-shared PROPERTIES OUTPUT_NAME qajson4c )

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION include
        FILES_MATCHING PATTERN "*.h" 
        PATTERN ".svn" EXCLUDE)

INSTALL(TARGETS qajson4c DESTINATION "lib")
INSTALL(TARGETS qajson4c-shared DESTINATION "lib")
        
SET_TARGET_PROPERTIES(qajson4c PROPERTIES LINKER_LANGUAGE C)
SET_TARGET_PROPERTIES(qajson4c-shared PROPERTIES LINKER_LANGUAGE C)
