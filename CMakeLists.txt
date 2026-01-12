cmake_minimum_required(VERSION 3.21)

SET(MY_GIT_TAG CACHE STRING "")


find_package(Git QUIET)

set(GIT_DIR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.git")
if(EXISTS ${GIT_DIR_PATH})
    if(Git_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE MY_GIT_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE # Removes trailing newline/whitespace
            RESULT_VARIABLE GIT_DESCRIBE_RESULT
            ERROR_QUIET
        )
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list ${MY_GIT_TAG}..HEAD --count
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_REVISION
            OUTPUT_STRIP_TRAILING_WHITESPACE # Removes trailing newline/whitespace
            RESULT_VARIABLE GIT_REV_LIST_RESULT
            ERROR_QUIET
        )
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_NAME
            OUTPUT_STRIP_TRAILING_WHITESPACE # Removes trailing newline/whitespace
            RESULT_VARIABLE GIT_DESCRIBE_RESULT
            ERROR_QUIET
        )
        set(REVISION ${GIT_REVISION})
    endif()
endif()

if(NOT MY_GIT_TAG MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "Git last tag '${MY_GIT_TAG}' is not in format X.Y.Z")
endif()

project(cfrds VERSION ${MY_GIT_TAG} DESCRIPTION "ColdFusion RDS Library" LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set(PROJECT_LIB_NAME cfrds)

include(GNUInstallDirs)

if(WIN32)
    # Define the location of the template and the output file
    set(RC_DLL_IN_FILE "version.rc.in")
    set(RC_DLL_OUT_FILE "version_dll.rc")
    set(RC_OUT_EXTENSION "dll")

    # Configure the file (replaces @VAR@ with CMake variable values)
    configure_file(
        ${RC_DLL_IN_FILE}
        ${RC_DLL_OUT_FILE}
        @ONLY # Only substitute variables in the @VAR@ format
    )

    set(RC_EXE_IN_FILE "version.rc.in")
    set(RC_EXE_OUT_FILE "version_exe.rc")
    set(RC_OUT_EXTENSION "exe")
    configure_file(
        ${RC_EXE_IN_FILE}
        ${RC_EXE_OUT_FILE}
        @ONLY # Only substitute variables in the @VAR@ format
    )
endif()

###
if(WIN32)
    add_executable(cfrds_cli
        cli/main.c
        cli/os_win.c cli/os.h
    )
else()
    add_executable(cfrds_cli
        cli/main.c
        cli/os_posix.c cli/os.h
    )
endif()

set_target_properties(cfrds_cli
    PROPERTIES OUTPUT_NAME "cfrds"
)
target_sources(cfrds_cli PRIVATE ${RC_EXE_OUT_FILE})

target_link_libraries(cfrds_cli PRIVATE
    libcfrds
)

if(WIN32)
    target_link_libraries(cfrds_cli PRIVATE
        wsock32
        ws2_32
    )
endif()

install(TARGETS cfrds_cli
)
###
add_library(libcfrds SHARED
    src/cfrds.c        include/cfrds.h
    src/wddx.c         include/wddx.h
    src/cfrds_buffer.c include/internal/cfrds_buffer.h
    src/cfrds_http.c   include/internal/cfrds_http.h
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/cfrds.pc.in"
    "${CMAKE_BINARY_DIR}/cfrds.pc"
    @ONLY
)

target_include_directories(libcfrds PUBLIC
    include
)

if(WIN32)
set_target_properties(libcfrds
    PROPERTIES OUTPUT_NAME "cfrds"
)
target_sources(libcfrds PRIVATE ${RC_DLL_OUT_FILE})
target_include_directories(libcfrds PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/libxml2/include
)
target_link_directories(libcfrds PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/libxml2
)
target_link_libraries(libcfrds PRIVATE
    wsock32
    ws2_32
    xml2
)
else()
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
target_link_libraries(libcfrds PRIVATE LibXml2::LibXml2)
endif()

set(LIBCFRDS_PUBLIC_HEADERS
    "include/cfrds.h"
    "include/wddx.h"
)

set_target_properties(libcfrds PROPERTIES
    PUBLIC_HEADER "${LIBCFRDS_PUBLIC_HEADERS}"
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "cfrds"
    VERSION ${MY_GIT_TAG}
)

add_custom_command(TARGET libcfrds POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
)

install(TARGETS libcfrds
    PUBLIC_HEADER
)

install(FILES ${CMAKE_BINARY_DIR}/cfrds.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
)
