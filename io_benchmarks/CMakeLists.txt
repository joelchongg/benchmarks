cmake_minimum_required(VERSION 3.15)
project(IOBenchmarks CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# compilation flags
add_compile_options(
    -O3
    -march=native
    -mtune=native
    -Wall
    -Wextra
    -Wpedantic
)

# mmap benchmark
add_executable(bench_mmap bench_mmap.cpp)

# io_uring benchmark
add_executable(bench_io_uring bench_io_uring.cpp)
target_link_libraries(bench_io_uring uring)     # links liburing