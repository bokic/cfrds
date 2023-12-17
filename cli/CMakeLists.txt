cmake_minimum_required(VERSION 3.15)

project(cfrds_cli VERSION 0.7.0 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(cfrds_cli
    main.c
)

set_target_properties(cfrds_cli PROPERTIES
    OUTPUT_NAME "cfrds"
)

target_link_libraries(cfrds_cli PRIVATE
    libcfrds
)
