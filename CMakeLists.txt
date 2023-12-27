cmake_minimum_required(VERSION 3.15)

project(cfrds VERSION 0.7.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

###
add_executable(cfrds
    cli/main.c
)

target_link_libraries(cfrds PRIVATE
    libcfrds
)
###
add_library(libcfrds SHARED
    src/cfrds.c        include/cfrds.h
    src/cfrds_buffer.c include/internal/cfrds_buffer.h
    src/cfrds_http.c   include/internal/cfrds_http.h
)

target_include_directories(libcfrds PUBLIC
    include
)

if(WIN32)
  target_link_libraries(libcfrds wsock32 ws2_32)
endif()

set_target_properties(libcfrds PROPERTIES
    PUBLIC_HEADER "include/cfrds.h"
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "cfrds"
    VERSION "0.7.0"
    SOVERSION "0.7"
)

install(TARGETS libcfrds
    PUBLIC_HEADER
)
