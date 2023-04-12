find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_test(NAME test.functional.local-server
        COMMAND Python3::Interpreter -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/pytest/test_local_server.py
            --local_server $<TARGET_FILE:server> --server_port 1540
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/tests/functional
        )