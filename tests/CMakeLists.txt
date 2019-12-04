cmake_minimum_required(VERSION 3.14)

project(dense_hash_map_tests)

add_executable(dense_hash_map_tests src/dense_hash_map_tests)
target_link_libraries(dense_hash_map_tests Catch2::Catch2)
target_link_libraries(dense_hash_map_tests dense_hash_map)

if(MSVC)
  target_compile_options(dense_hash_map_tests PRIVATE /W4 /WX)
else()
  target_compile_options(dense_hash_map_tests PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
