cmake_minimum_required(VERSION 3.21)

project(cfrds VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

if(WIN32)
    # Define the location of the template and the output file
    set(RC_IN_FILE "version.rc.in")
    set(RC_OUT_FILE "version.rc")

    # Configure the file (replaces @VAR@ with CMake variable values)
    configure_file(
        ${RC_IN_FILE}
        ${RC_OUT_FILE}
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
    src/wddx.c         include/internal/wddx.h
    src/cfrds_buffer.c include/internal/cfrds_buffer.h
    src/cfrds_http.c   include/internal/cfrds_http.h
)

target_include_directories(libcfrds PUBLIC
    include
)

if(WIN32)
set_target_properties(libcfrds
    PROPERTIES OUTPUT_NAME "cfrds"
)
target_sources(libcfrds PRIVATE ${RC_OUT_FILE})
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

set_target_properties(libcfrds PROPERTIES
    PUBLIC_HEADER "include/cfrds.h"
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "cfrds"
    VERSION "1.0.0"
    SOVERSION "1.0"
)

add_custom_command(TARGET libcfrds POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
)

install(TARGETS libcfrds
    PUBLIC_HEADER
)

add_subdirectory(python)
