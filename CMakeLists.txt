cmake_minimum_required(VERSION 3.15)

project(cfrds VERSION 0.8.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

pkg_search_module(libpcre2-8 REQUIRED libpcre2-8>=10.42)

###
add_executable(cfrds_cli
    cli/main.c
)

set_target_properties(cfrds_cli
    PROPERTIES OUTPUT_NAME "cfrds"
)

target_include_directories(cfrds_cli PRIVATE
    ${libpcre2-8_INCLUDE_DIRS}
)

target_link_directories(cfrds_cli PRIVATE
    ${libpcre2-8_LIBRARY_DIRS}
)

target_link_libraries(cfrds_cli PRIVATE
    ${libpcre2-8_LIBRARIES}
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
    VERSION "0.8.0"
    SOVERSION "0.8"
)

install(TARGETS libcfrds
    PUBLIC_HEADER
)
