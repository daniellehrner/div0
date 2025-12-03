# benchmarks/CMakeLists.txt - Google Benchmark integration

# Warn if benchmarks are being built in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR DIV0_SANITIZER_BUILD)
  message(WARNING "Building benchmarks in ${CMAKE_BUILD_TYPE} mode. "
                  "Performance measurements will not be accurate. "
                  "Use Release or RelWithDebInfo for valid benchmarks.")
endif()

# Create benchmark executable
add_executable(div0_bench
  "types/uint256_arithmetic_bench.cpp"
  "types/uint256_bitwise_bench.cpp"
  "types/uint256_shift_bench.cpp"
  "types/uint256_comparison_bench.cpp"
  "types/uint256_utility_bench.cpp"
  "evm/opcode_arithmetic_bench.cpp"
  "evm/opcode_comparison_bench.cpp"
  "evm/opcode_memory_bench.cpp"
  "evm/opcode_stack_bench.cpp"
  "crypto/keccak256_bench.cpp"
  "crypto/secp256k1_bench.cpp"
  "rlp/rlp_bench.cpp"
)

# Disable clang-tidy for benchmarks (Google Benchmark macros trigger many warnings)
set_target_properties(div0_bench PROPERTIES CXX_CLANG_TIDY "")

# Link against the libraries being benchmarked
target_link_libraries(div0_bench
  PRIVATE
    div0_types
    div0_evm
    div0_crypto
    div0_rlp
    benchmark::benchmark
    benchmark::benchmark_main
)

message(STATUS "Benchmarks configured with Google Benchmark")