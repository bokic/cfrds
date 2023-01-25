cmake_minimum_required(VERSION 3.15)

project(cfrds LANGUAGES C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

include_directories(
    ../include
)

add_library(cfrds SHARED
    cfrds.c        ../include/cfrds.h
    cfrds_buffer.c ../include/cfrds_buffer.h
    cfrds_http.c   ../include/cfrds_http.h
)

set_target_properties(cfrds PROPERTIES
    PUBLIC_HEADER "../include/cfrds.h;../include/cfrds_buffer.h"
    VERSION "0.5.1"
    SOVERSION "0.5"
)

install(TARGETS cfrds
    PUBLIC_HEADER
)
