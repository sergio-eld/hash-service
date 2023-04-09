
add_executable(test.unit.hashing hashing.cpp)
target_link_static_crt(test.unit.hashing)
target_link_libraries(test.unit.hashing
        PRIVATE
            hash_server
            GTest::gtest
        )

add_test(NAME test.unit.hashing COMMAND test.unit.hashing)