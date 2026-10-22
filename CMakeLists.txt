cmake_minimum_required(VERSION 3.16)
project(string_matching_comparison LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(search_algorithms STATIC src/karp_rabin.cpp src/boyer_moore.cpp)
target_include_directories(search_algorithms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
    target_compile_options(search_algorithms PRIVATE /W4)
else()
    target_compile_options(search_algorithms PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(run_tests tests/test_search.cpp)
target_link_libraries(run_tests PRIVATE search_algorithms)

add_executable(benchmark src/benchmark.cpp)
target_link_libraries(benchmark PRIVATE search_algorithms)

enable_testing()
add_test(NAME search_correctness COMMAND run_tests)
