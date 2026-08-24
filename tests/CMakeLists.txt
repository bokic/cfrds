find_package(cmocka REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(json-c REQUIRED)

add_executable(test_buffer test_buffer.c)
cfrds_enable_strict_warnings(test_buffer)
target_include_directories(test_buffer PRIVATE ../include ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_buffer PRIVATE libcfrds cmocka LibXml2::LibXml2 json-c::json-c)
add_test(NAME test_buffer COMMAND test_buffer)

add_executable(test_wddx test_wddx.c)
cfrds_enable_strict_warnings(test_wddx)
target_include_directories(test_wddx PRIVATE ../include ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_wddx PRIVATE libcfrds cmocka LibXml2::LibXml2 json-c::json-c)
add_test(NAME test_wddx COMMAND test_wddx)

add_executable(test_cpp test_cpp.cpp)
cfrds_enable_strict_warnings(test_cpp)
target_include_directories(test_cpp PRIVATE ../include ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_cpp PRIVATE libcfrds)
add_test(NAME test_cpp COMMAND test_cpp)

