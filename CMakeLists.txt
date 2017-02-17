
cmake_minimum_required(VERSION 2.8.12)

project(libqajson4c C CXX) # unfortunately the tests are written c++ like

function ( register_build_type_coverage )
    message( STATUS "** Adding build type: Coverage")
    SET(CMAKE_CXX_FLAGS_COVERAGE
        "-fprofile-arcs -ftest-coverage --coverage"
        CACHE
        STRING "Flags used by the C++ compiler during coverage builds."
        FORCE
        )
    SET(CMAKE_C_FLAGS_COVERAGE
        "-fprofile-arcs -ftest-coverage"
        CACHE
        STRING "Flags used by the C compiler during coverage builds."
        FORCE
        )
    SET(CMAKE_EXE_LINKER_FLAGS_COVERAGE
        ""
        CACHE
        STRING "Flags used for linking binaries during coverage builds."
        FORCE
        )
    SET(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
        ""
        CACHE
        STRING "Flags used by the shared libraries linker during coverage builds."
        FORCE
        )
    MARK_AS_ADVANCED(
        CMAKE_CXX_FLAGS_COVERAGE
        CMAKE_C_FLAGS_COVERAGE
        CMAKE_EXE_LINKER_FLAGS_COVERAGE
        CMAKE_SHARED_LINKER_FLAGS_COVERAGE )
endfunction ( register_build_type_coverage )

register_build_type_coverage()

set(CMAKE_C_FLAGS "-ansi -pedantic -pedantic-errors -Wall -Wextra")
#set(CMAKE_C_FLAGS " -pedantic -pedantic-errors -Wall -Wextra")

find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})
if(BUILD_DOCUMENTATION)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()

    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION doc)
endif()

if( EXISTS ${CMAKE_TOOLCHAIN_FILE} )
    get_filename_component(USE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE} NAME)
    if (${USE_TOOLCHAIN_FILE} MATCHES "^toolchain\\..+\\.cmake$")
        message(STATUS "match")
        STRING(REGEX REPLACE "toolchain\\.(.+)\\.cmake" "\\1" USE_TOOLCHAIN_NAME ${USE_TOOLCHAIN_FILE})
    else()
        SET(USE_TOOLCHAIN_NAME ${USE_TOOLCHAIN_FILE})
    endif()
endif()

if (${CMAKE_EXTRA_GENERATOR} MATCHES "Eclipse CDT4" )
    set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")
    set(BASE_DIR ${PROJECT_ROOT_DIR}/build)
    set(CMAKE_BINARY_DIR ${BASE_DIR} CACHE INTERNAL "")
    set(USE_OUTPUT_DIR ${BASE_DIR}/${USE_TOOLCHAIN_NAME} CACHE INTERNAL "")
else()
    set(BASE_DIR ${CMAKE_BINARY_DIR}/build)
    set(USE_OUTPUT_DIR ${BASE_DIR}/${USE_TOOLCHAIN_NAME} CACHE INTERNAL "")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${USE_OUTPUT_DIR}/lib CACHE INTERNAL "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${USE_OUTPUT_DIR}/lib CACHE INTERNAL "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${USE_OUTPUT_DIR}/bin CACHE INTERNAL "")

add_subdirectory(src)
add_subdirectory(src-test)

