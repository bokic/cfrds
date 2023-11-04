cmake_minimum_required(VERSION 3.15)

project(libcfrds LANGUAGES C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

include_directories(BEFORE
    ../include
)

add_library(libcfrds SHARED
    cfrds.c        ../include/cfrds.h
    cfrds_buffer.c ../include/internal/cfrds_buffer.h
    cfrds_http.c   ../include/internal/cfrds_http.h
)

if(WIN32)
  target_link_libraries(libcfrds wsock32 ws2_32)
endif()

set_target_properties(libcfrds PROPERTIES
    PUBLIC_HEADER "../include/cfrds.h"
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "cfrds"
    VERSION "0.6.0"
    SOVERSION "0.6"
)

install(TARGETS libcfrds
    PUBLIC_HEADER
)
