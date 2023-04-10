
add_executable(test.unit.hashing hashing.cpp)
target_link_static_crt(test.unit.hashing)
target_link_libraries(test.unit.hashing
        PRIVATE
            hash_server
            GTest::gtest
        )

# set(CMAKE_DEBUG_POSTFIX _d) doesn't work for some reason
set_target_properties(test.unit.hashing
        PROPERTIES
            DEBUG_POSTFIX _d
        )

add_test(NAME test.unit.hashing COMMAND test.unit.hashing)