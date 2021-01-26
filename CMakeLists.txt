cmake_minimum_required(VERSION 3.14)
project(dense_hash_map)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dense_hash_map INTERFACE)
target_include_directories(dense_hash_map INTERFACE include/)
add_library(JGuegant::dense_hash_map ALIAS dense_hash_map)

if(JGUEGANT_DENSE_HASH_MAP_TESTS)
    add_subdirectory(thirdparty/catch2)
    add_subdirectory(tests)
endif()