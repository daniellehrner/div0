cmake_minimum_required(VERSION 4.1)

project(div0
  VERSION 0.1.0
  DESCRIPTION "High-performance EVM implementation"
  LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

# ============================================================================
# Build Options
# ============================================================================

# Detect if we're building for bare-metal (freestanding)
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
  set(DIV0_FREESTANDING ON)
  message(STATUS "Building for bare-metal (freestanding)")
else()
  set(DIV0_FREESTANDING OFF)
endif()

# Use lld linker for faster linking (Clang only, skip for freestanding)
if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT DIV0_FREESTANDING)
  add_link_options("-fuse-ld=lld")
  message(STATUS "Using lld linker")
endif()

option(DIV0_BUILD_TESTS "Build tests" ON)
option(DIV0_FREESTANDING_TESTS "Build tests for freestanding targets (requires QEMU)" ON)
option(DIV0_ENABLE_LTO "Enable link-time optimization for Release builds" ON)

# QEMU emulator for cross-compiled tests
set(DIV0_QEMU_RISCV64 "qemu-riscv64" CACHE STRING "QEMU RISC-V 64-bit emulator")

# ============================================================================
# CMake Modules
# ============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CompilerWarnings)
include(Sanitizers)
include(Dependencies)

# ============================================================================
# Link-Time Optimization (LTO)
# ============================================================================

if(DIV0_ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT DIV0_FREESTANDING)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
  if(IPO_SUPPORTED)
    message(STATUS "Link-time optimization (LTO) enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  else()
    message(WARNING "LTO requested but not supported: ${IPO_ERROR}")
  endif()
endif()

# ============================================================================
# Debug Symbols in Release Builds
# ============================================================================
# Keep debug symbols for crash debugging without impacting performance.

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-g)
  message(STATUS "Debug symbols enabled in Release build")
endif()

# ============================================================================
# Freestanding Build Configuration
# ============================================================================

set(DIV0_FREESTANDING_FLAGS
  -ffreestanding
  -fno-builtin
)

# ============================================================================
# Helper Function
# ============================================================================

function(div0_target_options target)
  target_compile_options(${target} PRIVATE ${DIV0_WARNING_FLAGS})

  if(DIV0_FREESTANDING)
    target_compile_options(${target} PRIVATE ${DIV0_FREESTANDING_FLAGS})
    target_compile_definitions(${target} PRIVATE DIV0_FREESTANDING=1)
    if(DIV0_BUILD_PICOLIBC)
      target_include_directories(${target} SYSTEM PRIVATE ${DIV0_PICOLIBC_INCLUDE_DIR})
      add_dependencies(${target} picolibc_ext)
    endif()
  endif()
endfunction()

# ============================================================================
# Libraries
# ============================================================================

# Memory library (arena allocator) - no dependencies
add_library(div0_mem STATIC
  src/mem/arena.c
)
target_include_directories(div0_mem PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(div0_mem PUBLIC stc_headers)
div0_target_options(div0_mem)

# Types library (uint256, bytes32, hash, address, bytes) - bytes uses arena
add_library(div0_types STATIC
  src/types/uint256.c
  src/types/bytes32.c
  src/types/hash.c
  src/types/address.c
  src/types/bytes.c
  src/util/hex.c
)
target_include_directories(div0_types PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(div0_types PUBLIC div0_mem)
div0_target_options(div0_types)

# EVM library (stack, interpreter, memory, call frames) - depends on types, mem, state
add_library(div0_evm STATIC
  src/evm/evm.c
  src/evm/memory.c
  src/evm/call_op.c
  src/evm/opcodes/call.c
)
target_link_libraries(div0_evm PUBLIC div0_types div0_mem)
div0_target_options(div0_evm)
# Computed goto dispatch tables intentionally override default entries
target_compile_options(div0_evm PRIVATE -Wno-initializer-overrides)

# Crypto library (Keccak-256, secp256k1) - depends on types
add_library(div0_crypto STATIC
  src/crypto/keccak256.c
  src/crypto/secp256k1.c
)
target_include_directories(div0_crypto PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(div0_crypto
  PUBLIC div0_types
  PRIVATE xkcp secp256k1
)
div0_target_options(div0_crypto)
if(DIV0_FREESTANDING)
  add_dependencies(div0_crypto secp256k1_ext xkcp_build)
endif()

# RLP library - depends on types
add_library(div0_rlp STATIC
  src/rlp/helpers.c
  src/rlp/encode.c
  src/rlp/decode.c
)
target_include_directories(div0_rlp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(div0_rlp PUBLIC div0_types)
div0_target_options(div0_rlp)

# Trie library (Merkle Patricia Trie) - depends on types, crypto, rlp
add_library(div0_trie STATIC
  src/trie/nibbles.c
  src/trie/hex_prefix.c
  src/trie/node.c
  src/trie/mpt.c
  src/trie/mpt_memory.c
)
target_include_directories(div0_trie PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(div0_trie PUBLIC div0_types div0_mem div0_crypto div0_rlp)
div0_target_options(div0_trie)

# State library (world state, accounts) - depends on types, crypto, rlp, trie
add_library(div0_state STATIC
  src/state/account.c
  src/state/world_state.c
)
target_include_directories(div0_state PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(div0_state PUBLIC div0_types div0_crypto div0_rlp div0_trie stc_headers)
div0_target_options(div0_state)

# Ethereum library (transactions) - depends on types, crypto, rlp
add_library(div0_ethereum STATIC
  src/ethereum/transaction/rlp.c
  src/ethereum/transaction/signer.c
)
target_include_directories(div0_ethereum PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(div0_ethereum PUBLIC div0_types div0_crypto div0_rlp)
div0_target_options(div0_ethereum)

# Block Executor library - depends on types, evm, state, ethereum, crypto, rlp
add_library(div0_executor STATIC
  src/executor/address.c
  src/executor/block_executor.c
  src/executor/intrinsic_gas.c
  src/executor/validation.c
)
target_include_directories(div0_executor PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(div0_executor PUBLIC div0_types div0_evm div0_state div0_ethereum div0_crypto div0_rlp)
div0_target_options(div0_executor)

# ============================================================================
# JSON Library (hosted only - not for bare-metal/zk builds)
# ============================================================================

if(NOT DIV0_FREESTANDING)
  add_library(div0_json STATIC
    src/json/parse.c
    src/json/write.c
    src/t8n/alloc.c
    src/t8n/env.c
    src/t8n/txs.c
    src/t8n/result.c
  )
  target_include_directories(div0_json PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(div0_json
    PUBLIC div0_types div0_mem div0_ethereum div0_state
    PRIVATE yyjson
  )
  div0_target_options(div0_json)
endif()

# Convenience interface library linking all components
add_library(div0 INTERFACE)
target_link_libraries(div0 INTERFACE div0_types div0_mem div0_evm div0_crypto div0_rlp div0_trie div0_state div0_ethereum div0_executor)

# ============================================================================
# CLI Executable (hosted only)
# ============================================================================

if(NOT DIV0_FREESTANDING)
  # Generate version header
  configure_file(
    src/cli/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/cli/version.h
  )

  # CLI library
  add_library(div0_cli STATIC
    src/cli/crash_handler.c
    src/cli/t8n/t8n_command.c
  )
  target_include_directories(div0_cli
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src/cli
  )
  target_link_libraries(div0_cli
    PUBLIC div0 div0_json
    PRIVATE argparse libbacktrace
  )
  div0_target_options(div0_cli)

  # CLI executable
  add_executable(div0_exe src/cli/main.c)
  set_target_properties(div0_exe PROPERTIES OUTPUT_NAME div0)
  target_include_directories(div0_exe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cli
    ${CMAKE_CURRENT_BINARY_DIR}/include
  )
  target_link_libraries(div0_exe PRIVATE div0_cli argparse)
  # Export symbols for readable crash traces
  set_target_properties(div0_exe PROPERTIES ENABLE_EXPORTS ON)
  div0_target_options(div0_exe)
endif()

# ============================================================================
# Tests
# ============================================================================

if(DIV0_BUILD_TESTS)
  enable_testing()

  # Core test sources - work on both hosted and freestanding builds
  set(DIV0_TEST_SOURCES
    tests/test_div0.c
    # types tests
    tests/types/test_uint256.c
    tests/types/test_bytes32.c
    tests/types/test_hash.c
    tests/types/test_address.c
    tests/types/test_bytes.c
    # mem tests
    tests/mem/test_arena.c
    # util tests
    tests/util/test_hex.c
    # evm tests
    tests/evm/test_stack.c
    tests/evm/test_stack_pool.c
    tests/evm/test_evm.c
    tests/evm/test_opcodes_arithmetic.c
    tests/evm/test_opcodes_bitwise.c
    tests/evm/test_opcodes_comparison.c
    tests/evm/test_opcodes_context.c
    tests/evm/test_opcodes_stack.c
    # crypto tests
    tests/crypto/test_keccak256.c
    tests/crypto/test_secp256k1.c
    # rlp tests
    tests/rlp/test_rlp.c
    # trie tests
    tests/trie/test_nibbles.c
    tests/trie/test_hex_prefix.c
    tests/trie/test_node.c
    tests/trie/test_mpt.c
    # state tests
    tests/state/test_account.c
    tests/state/test_world_state.c
    # ethereum tests
    tests/ethereum/transaction/test_transaction.c
    # executor tests
    tests/executor/test_block_executor.c
  )

  # Hosted-only test sources (require file I/O, JSON, etc.)
  # These tests are excluded from freestanding/RISC-V builds
  set(DIV0_HOSTED_TEST_SOURCES
    tests/json/test_json.c
    tests/t8n/test_t8n.c
  )

  if(DIV0_FREESTANDING)
    # Freestanding tests - need special linking for QEMU execution
    if(DIV0_FREESTANDING_TESTS AND DIV0_BUILD_PICOLIBC)
      add_executable(div0_tests
        ${DIV0_TEST_SOURCES}
        tests/qemu_riscv64.c
        ${unity_SOURCE_DIR}/src/unity.c
      )

      # picolibc include MUST come first for standard headers
      target_include_directories(div0_tests SYSTEM BEFORE PRIVATE
        ${DIV0_PICOLIBC_INCLUDE_DIR}
      )
      target_include_directories(div0_tests PRIVATE
        ${unity_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
      )

      add_dependencies(div0_tests picolibc_ext compiler_rt_ext xkcp_build secp256k1_ext)

      target_compile_options(div0_tests PRIVATE ${DIV0_WARNING_FLAGS} -Wno-conversion)
      target_compile_definitions(div0_tests PRIVATE DIV0_FREESTANDING=1)

      # Link statically with picolibc and compiler-rt for QEMU user-mode execution
      # The toolchain file sets CMAKE_C_LINK_EXECUTABLE to wrap everything in --start-group/--end-group
      target_link_libraries(div0_tests PRIVATE
        div0_types
        div0_mem
        div0_evm
        div0_crypto
        div0_rlp
        div0_trie
        div0_state
        div0_ethereum
        div0_executor
        "${DIV0_PICOLIBC_LIB}"
        "${DIV0_COMPILER_RT_LIB}"
      )
      target_link_options(div0_tests PRIVATE
        -static
        -nostdlib
      )

      add_test(
        NAME div0_tests
        COMMAND ${DIV0_QEMU_RISCV64} ${CMAKE_CURRENT_BINARY_DIR}/div0_tests
      )
    endif()
  else()
    # Hosted tests - normal build with JSON tests
    add_executable(div0_tests
      ${DIV0_TEST_SOURCES}
      ${DIV0_HOSTED_TEST_SOURCES}
      ${unity_SOURCE_DIR}/src/unity.c
    )

    target_include_directories(div0_tests PRIVATE
      ${unity_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(div0_tests PRIVATE div0 div0_json)
    div0_target_options(div0_tests)

    add_test(NAME div0_tests COMMAND div0_tests)
  endif()
endif()

# =============================================================================
# Benchmarks (optional, only for hosted builds)
# =============================================================================

option(DIV0_BUILD_BENCHMARKS "Build benchmark executables" ON)

if(DIV0_BUILD_BENCHMARKS AND NOT DIV0_FREESTANDING)
  add_subdirectory(benchmarks)
endif()