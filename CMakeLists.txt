cmake_minimum_required(VERSION 3.21)

project(cfrds VERSION 0.9.7 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

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
    src/wddx.c         include/wddx.h
    src/cfrds_buffer.c include/internal/cfrds_buffer.h
    src/cfrds_http.c   include/internal/cfrds_http.h
)

if(WIN32)
set_target_properties(libcfrds
    PROPERTIES OUTPUT_NAME "cfrds"
)
endif()

target_include_directories(libcfrds PUBLIC
    include
)

if(WIN32)
target_include_directories(libcfrds PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/libxml2/include
)
target_link_directories(libcfrds PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/libxml2
)
else()
target_include_directories(libcfrds PRIVATE
    /usr/include/libxml2
)
endif()

target_link_libraries(libcfrds PRIVATE xml2)

if(WIN32)
    target_link_libraries(libcfrds PRIVATE
        wsock32
        ws2_32
    )
endif()

set_target_properties(libcfrds PROPERTIES
    PUBLIC_HEADER "include/cfrds.h"
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "cfrds"
    VERSION "0.9.7"
    SOVERSION "0.9"
)

install(TARGETS libcfrds
    PUBLIC_HEADER
)

subdirs(python)
