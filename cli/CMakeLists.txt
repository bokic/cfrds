cmake_minimum_required(VERSION 3.5)

project(cfrds_cli VERSION 0.5 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(
    ../include
)

add_executable(cfrds_cli
    main.c
)

set_target_properties(cfrds_cli PROPERTIES
    OUTPUT_NAME "cfrds"
)

target_link_libraries(cfrds_cli PRIVATE
    libcfrds
)
