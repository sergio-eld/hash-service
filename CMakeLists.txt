# Ubuntu 20.04 has 3.16
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(hash-service VERSION 0.0.1 LANGUAGES CXX)

option(DEBUG_ASIO "Enable ASIO debugging for server" OFF)
message(STATUS "DEBUG_ASIO: ${DEBUG_ASIO}")

option(BUILD_TESTS "Build test suite" ON)
message(STATUS "BUILD_TESTS: ${BUILD_TESTS}")

option(UNIT_TESTS "Build unit tests" ON)
message(STATUS "UNIT_TESTS: ${UNIT_TESTS}")

option(FUNCTIONAL_TESTS "Functional tests" ON)
message(STATUS "FUNCTIONAL_TESTS: ${FUNCTIONAL_TESTS}")

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(clean_build)
include(target_link_static_crt)

find_package(ASIO REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(hash_server INTERFACE)
target_link_libraries(hash_server
        INTERFACE
            ASIO::ASIO
            OpenSSL::SSL
        )
target_include_directories(hash_server
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

add_executable(server src/main.cpp)
target_link_static_crt(server)
target_link_libraries(server
        PRIVATE
            hash_server
        )
if (${DEBUG_ASIO})
    target_compile_definitions(server PRIVATE DEBUG_ASIO)
endif ()

# set(CMAKE_DEBUG_POSTFIX _d) doesn't work for some reason
set_target_properties(server
        PROPERTIES
            DEBUG_POSTFIX _d
        )

# TODO: cmake option for BUILD_TESTS
if (${BUILD_TESTS})
    enable_testing()
    add_subdirectory(tests)
endif ()


# Deployment
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_INSTALL_PREFIX})
include(CPack)

if (WIN32)
# TODO: deploy dlls. $<TARGET_RUNTIME_DLLS:server> only from cmake 3.21
#    add_custom_target(deploy.server
#            TARGET server POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:server> $<TARGET_FILE_DIR:server>
#            )
endif()


install(TARGETS server DESTINATION bin)
