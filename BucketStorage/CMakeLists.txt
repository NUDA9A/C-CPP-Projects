cmake_minimum_required(VERSION 3.27)
project(ct_c24_lw_containers_NUDA9A)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory("./googletest")
include_directories("./googletest/googletest/include")
include_directories("./googletest/googlemock/include")
include_directories(.)

add_executable(ct_c24_lw_containers_NUDA9A
        helpers.h
        tests.cpp
        bucket_storage.hpp
        bucket_iterator.hpp
        my_stack.hpp
        structs.hpp
)

target_link_libraries(ct_c24_lw_containers_NUDA9A gtest gtest_main)
target_link_libraries(ct_c24_lw_containers_NUDA9A gmock gmock_main)
