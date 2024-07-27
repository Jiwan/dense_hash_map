cmake_minimum_required(VERSION 3.14)
project(dense_hash_map)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)
option(DENSE_HASH_MAP_BUILD_TEST "Build Dense Hash Map tests" ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dense_hash_map INTERFACE)
target_include_directories(dense_hash_map INTERFACE include/)
add_library(JGuegant::dense_hash_map ALIAS dense_hash_map)

if (is_top_level OR ${DENSE_HASH_MAP_BUILD_TEST})
    add_subdirectory(thirdparty/catch2_ext)
    add_subdirectory(tests)
endif()
