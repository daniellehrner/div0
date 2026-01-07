# Benchmarks for div0

# uint256 benchmarks
add_executable(uint256_bench
  uint256_bench.c
)

target_include_directories(uint256_bench PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(uint256_bench PRIVATE
  div0_types
)

# Enable optimizations for benchmarks even in debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(uint256_bench PRIVATE -O2)
endif()

# stack benchmarks
add_executable(stack_bench
  stack_bench.c
)

target_include_directories(stack_bench PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(stack_bench PRIVATE
  div0_evm
  div0_types
  div0_mem
)

# Enable optimizations for benchmarks even in debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(stack_bench PRIVATE -O2)
endif()
