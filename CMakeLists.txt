cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(hash-service VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(sb.dummy sb.dummy.cpp)
install(TARGETS sb.dummy DESTINATION bin)

set(CPACK_PACKAGE_DIRECTORY ${CMAKE_INSTALL_PREFIX})



if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)