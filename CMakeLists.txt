cmake_minimum_required(VERSION 3.15)
project(SmartPointersTests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tests
        test/doctest_main.cpp
        test/unique_ptr_test.cpp
        test/shared_ptr_test.cpp
)

target_include_directories(tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/test
)