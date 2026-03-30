find_package(cmocka REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(json-c REQUIRED)

add_executable(test_buffer test_buffer.c ../src/cfrds_buffer.c ../src/wddx.c ../src/cfrds.c ../src/cfrds_http.c)
target_include_directories(test_buffer PRIVATE ../include)
target_link_libraries(test_buffer PRIVATE cmocka LibXml2::LibXml2 json-c::json-c)
add_test(NAME test_buffer COMMAND test_buffer)

add_executable(test_wddx test_wddx.c ../src/cfrds_buffer.c ../src/wddx.c ../src/cfrds.c ../src/cfrds_http.c)
target_include_directories(test_wddx PRIVATE ../include)
target_link_libraries(test_wddx PRIVATE cmocka LibXml2::LibXml2 json-c::json-c)
add_test(NAME test_wddx COMMAND test_wddx)
