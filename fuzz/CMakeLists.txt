# Fuzz targets using libFuzzer
if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Fuzzing requires Clang compiler (-fsanitize=fuzzer)")
endif()

add_executable(fuzz_rds_string
    cfrds_fuzz.c
    ${CMAKE_SOURCE_DIR}/src/cfrds.c
    ${CMAKE_SOURCE_DIR}/src/cfrds_server.c
    ${CMAKE_SOURCE_DIR}/src/cfrds_file.c
    ${CMAKE_SOURCE_DIR}/src/cfrds_sql.c
    ${CMAKE_SOURCE_DIR}/src/cfrds_debugger.c
    ${CMAKE_SOURCE_DIR}/src/cfrds_security_analyzer.c
    ${CMAKE_SOURCE_DIR}/src/cfrds_http.c
)

target_compile_options(fuzz_rds_string PRIVATE
    -fsanitize=fuzzer,address,undefined
    -g
    -O1
    -fno-omit-frame-pointer
    -Wall
    -Wextra
)

target_link_options(fuzz_rds_string PRIVATE
    -fsanitize=fuzzer,address,undefined
)

target_include_directories(fuzz_rds_string PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/internal
)

target_link_libraries(fuzz_rds_string PRIVATE
    LibXml2::LibXml2
    json-c::json-c
)
