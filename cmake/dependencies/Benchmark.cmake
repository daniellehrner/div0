# Benchmark.cmake - Google Benchmark framework
#

include(FetchContent)

message(STATUS "Fetching Google Benchmark...")
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.1
  GIT_SHALLOW    ON
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable gtest in benchmark" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)

FetchContent_MakeAvailable(benchmark)
