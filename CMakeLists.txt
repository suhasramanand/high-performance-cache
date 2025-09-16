cmake_minimum_required(VERSION 3.16)
project(HighPerformanceCache VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# Include directories
include_directories(include)

# Find required packages
find_package(Threads REQUIRED)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Source files
set(CACHE_SOURCES
    src/cache.cpp
    src/memory_allocator.cpp
    src/object_pool.cpp
    src/lru_cache.cpp
    src/tcp_server.cpp
    src/thread_pool.cpp
    src/protocol.cpp
)

set(CACHE_HEADERS
    include/cache.h
    include/memory_allocator.h
    include/object_pool.h
    include/lru_cache.h
    include/tcp_server.h
    include/thread_pool.h
    include/protocol.h
)

# Create library
add_library(cache_lib ${CACHE_SOURCES} ${CACHE_HEADERS})
target_link_libraries(cache_lib Threads::Threads)

# Main server executable
add_executable(cache_server src/main.cpp)
target_link_libraries(cache_server cache_lib)

# Client executable
add_executable(cache_client src/client.cpp)
target_link_libraries(cache_client cache_lib)

# Benchmark executable
add_executable(cache_benchmark src/benchmark.cpp)
target_link_libraries(cache_benchmark cache_lib)

# Unit tests
add_executable(cache_tests tests/test_cache.cpp tests/test_memory_allocator.cpp tests/test_lru_cache.cpp)
target_link_libraries(cache_tests cache_lib gtest_main)
target_include_directories(cache_tests PRIVATE include)

# Enable testing
enable_testing()
add_test(NAME CacheTests COMMAND cache_tests)
