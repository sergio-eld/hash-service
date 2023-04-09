cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(hash-service VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

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

# TODO: cmake option for BUILD_TESTS
enable_testing()
find_package(GTest REQUIRED)
add_subdirectory(tests/unit)


# Deployment
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_INSTALL_PREFIX})
include(CPack)

install(TARGETS server DESTINATION bin)
