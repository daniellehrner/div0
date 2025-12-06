cmake_minimum_required(VERSION 3.28)
project(div0 VERSION 0.1.0 LANGUAGES C CXX)

# ============================================================================
# Project Configuration
# ============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable C++20 module scanning - we're not using modules, and the .modmap
# references in compile_commands.json break clang-tidy
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Export compile commands for clang-tidy and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

# Use libc++ on Linux Clang to match macOS behavior
# Exception: Fuzz builds must use libstdc++ because libclang_rt.fuzzer is compiled with it
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT APPLE AND NOT DIV0_BUILD_FUZZ)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  add_link_options(-stdlib=libc++)
  message(STATUS "Using libc++ on Linux")
endif()

# Use lld linker for faster linking (Clang only)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if(APPLE)
    add_link_options("-fuse-ld=/opt/homebrew/bin/ld64.lld")
  else()
    add_link_options("-fuse-ld=lld")
  endif()
  message(STATUS "Using lld linker")
endif()

# Force static linking for all dependencies
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ============================================================================
# Project Options
# ============================================================================

option(DIV0_BUILD_TESTS "Build unit tests" ON)
option(DIV0_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(DIV0_BUILD_FUZZ "Build fuzzing targets" OFF)
option(DIV0_ENABLE_LTO "Enable link-time optimization for Release builds" ON)
option(DIV0_ENABLE_CLANG_TIDY "Run clang-tidy during build" ON)

# ============================================================================
# Include CMake Modules
# ============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Load compiler warnings configuration
include(CompilerWarnings)

# Load sanitizer configurations
include(Sanitizers)

# Load dependencies (GTest, Google Benchmark)
include(Dependencies)

# ============================================================================
# clang-tidy Integration
# ============================================================================

if(DIV0_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
    message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
  else()
    message(WARNING "clang-tidy requested but not found")
  endif()
endif()

# ============================================================================
# Link-Time Optimization (LTO)
# ============================================================================

if(DIV0_ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
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
# Add debug symbols to Release builds for crash debugging.
# This enables human-readable stack traces from the crash handler
# without significantly impacting performance.

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-g)
  message(STATUS "Debug symbols enabled in Release build")
endif()

# ============================================================================
# Library: div0_types
# ============================================================================
# Core types: uint256, addresses, hashes, etc.

add_library(div0_types STATIC
    include/div0/types/address.h
    include/div0/types/bytes.h
    include/div0/types/bytes32.h
    include/div0/types/hash.h
    include/div0/types/uint256.h
    src/types/hash.cpp
    src/types/uint256_divmod.cpp
    src/types/uint256_signed.cpp
    src/types/uint256_exp.cpp
    src/types/uint256_modular.cpp
)

target_include_directories(div0_types
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(div0_types PRIVATE ${DIV0_WARNING_FLAGS})

# ============================================================================
# Library: div0_utils
# ============================================================================
# Utility functions like byte manipulation

add_library(div0_utils INTERFACE
    include/div0/utils/bytes.h
)

target_include_directories(div0_utils
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ============================================================================
# Library: div0_rlp
# ============================================================================
# RLP (Recursive Length Prefix) encoding and decoding

add_library(div0_rlp STATIC
    include/div0/rlp/helpers.h
    include/div0/rlp/encode.h
    include/div0/rlp/decode.h
    include/div0/rlp/rlp.h
    src/rlp/encode.cpp
    src/rlp/decode.cpp
)

target_include_directories(div0_rlp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(div0_rlp
  PUBLIC
    div0_types
)

target_compile_options(div0_rlp PRIVATE ${DIV0_WARNING_FLAGS})

# ============================================================================
# Library: div0_crypto
# ============================================================================
# Cryptographic primitives (Keccak-256, future: secp256k1, BLS)

add_library(div0_crypto STATIC
    include/div0/crypto/keccak256.h
    src/crypto/keccak256.cpp
    include/div0/crypto/secp256k1.h
    src/crypto/secp256k1.cpp
)

target_include_directories(div0_crypto
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(div0_crypto
  PUBLIC
    div0_types    # For Uint256 in public API
    div0_utils    # For byte swapping utilities
  PRIVATE
    xkcp          # XKCP is implementation detail
    secp256k1     # ECDSA signature recovery
)

target_compile_options(div0_crypto PRIVATE ${DIV0_WARNING_FLAGS})

# ============================================================================
# Library: div0_trie
# ============================================================================
# Merkle Patricia Trie implementation

add_library(div0_trie STATIC
    include/div0/trie/nibbles.h
    include/div0/trie/hex_prefix.h
    include/div0/trie/node.h
    include/div0/trie/mpt.h
    src/trie/nibbles.cpp
    src/trie/hex_prefix.cpp
    src/trie/node.cpp
    src/trie/mpt.cpp
)

target_include_directories(div0_trie
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(div0_trie
  PUBLIC
    div0_types
  PRIVATE
    div0_rlp
    div0_crypto
)

target_compile_options(div0_trie PRIVATE ${DIV0_WARNING_FLAGS})

# ============================================================================
# Library: div0_ethereum
# ============================================================================
# Ethereum-specific consensus types and utilities

add_library(div0_ethereum STATIC
    include/div0/ethereum/account.h
    include/div0/ethereum/roots.h
    include/div0/ethereum/storage_slot.h
    include/div0/ethereum/storage_value.h
    include/div0/ethereum/bloom.h
    include/div0/ethereum/receipt.h
    include/div0/ethereum/transaction/types.h
    include/div0/ethereum/transaction/legacy_tx.h
    include/div0/ethereum/transaction/eip2930_tx.h
    include/div0/ethereum/transaction/eip1559_tx.h
    include/div0/ethereum/transaction/eip4844_tx.h
    include/div0/ethereum/transaction/eip7702_tx.h
    include/div0/ethereum/transaction/transaction.h
    include/div0/ethereum/transaction/rlp.h
    include/div0/ethereum/transaction/hex.h
    include/div0/ethereum/transaction/json.h
    src/ethereum/account.cpp
    src/ethereum/roots.cpp
    src/ethereum/bloom.cpp
    src/ethereum/receipt.cpp
    src/ethereum/transaction/rlp.cpp
    src/ethereum/transaction/legacy_tx.cpp
    src/ethereum/transaction/eip2930_tx.cpp
    src/ethereum/transaction/eip1559_tx.cpp
    src/ethereum/transaction/eip4844_tx.cpp
    src/ethereum/transaction/eip7702_tx.cpp
    src/ethereum/transaction/transaction.cpp
    src/ethereum/transaction/hex.cpp
    src/ethereum/transaction/json.cpp
)

target_include_directories(div0_ethereum
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(div0_ethereum
  PUBLIC
    div0_types
    div0_trie
    simdjson
  PRIVATE
    div0_rlp
    div0_crypto
)

# Note: div0_ethereum uses evm::Log from execution_result.h
# This creates a minor dependency, but it's header-only so we just need the include path
target_include_directories(div0_ethereum
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_options(div0_ethereum PRIVATE ${DIV0_WARNING_FLAGS})

# ============================================================================
# Library: div0_db
# ============================================================================
# Generic key-value database abstraction

add_library(div0_db STATIC
    include/div0/db/database.h
    include/div0/db/memory_database.h
    src/db/memory_database.cpp
)

target_include_directories(div0_db
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(div0_db PRIVATE ${DIV0_WARNING_FLAGS})

# ============================================================================
# Library: div0_state
# ============================================================================
# EVM state management (storage, accounts, code)

add_library(div0_state STATIC
    include/div0/state/storage_provider.h
    include/div0/state/account_provider.h
    include/div0/state/code_provider.h
    include/div0/state/state_context.h
    include/div0/state/memory_storage_provider.h
    src/state/memory_storage_provider.cpp
    include/div0/state/memory_account_provider.h
    include/div0/state/memory_code_provider.h
)

target_include_directories(div0_state
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(div0_state
  PUBLIC
    div0_types
    div0_ethereum
    div0_utils
    div0_db
)

target_compile_options(div0_state PRIVATE ${DIV0_WARNING_FLAGS})

# ============================================================================
# Library: div0_evm
# ============================================================================
# EVM interpreter, opcodes, stack, memory

add_library(div0_evm STATIC
    include/div0/evm/evm.h
    include/div0/evm/stack.h
    include/div0/evm/memory.h
    include/div0/evm/gas/dynamic_costs.h
    include/div0/evm/gas/schedule.h
    src/evm/evm.cpp
    src/evm/memory.cpp
    src/evm/opcodes.h
    src/evm/opcodes/arithmetic.h
    include/div0/evm/forks.h
    src/evm/gas/static_costs.h
    src/evm/opcodes/op_helpers.h
    src/evm/opcodes/comparison.h
    src/evm/opcodes/memory.h
    src/evm/opcodes/push.h
    src/evm/opcodes/storage.h
    src/evm/opcodes/swap.h
    src/evm/opcodes/dup.h
    src/evm/opcodes/keccak256.h
    include/div0/evm/call_frame.h
    include/div0/evm/call_frame_pool.h
    include/div0/evm/stack_pool.h
    include/div0/evm/memory_pool.h
    include/div0/evm/call_params.h
)

target_include_directories(div0_evm
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(div0_evm
  PUBLIC
    div0_types
    div0_state
    div0_crypto
)

target_compile_options(div0_evm PRIVATE ${DIV0_WARNING_FLAGS})

# ============================================================================
# Library: div0_cli
# ============================================================================
# CLI commands (t8n, t9n, b11r) - NOT a public API

# Generate version.h from template
configure_file(
    src/cli/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
    @ONLY
)

add_library(div0_cli STATIC
    src/cli/crash_handler.cpp
    src/cli/t8n/t8n_command.cpp
)

target_include_directories(div0_cli
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cli
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(div0_cli
  PUBLIC
    div0_evm
    div0_trie
    CLI11::CLI11
  PRIVATE
    libbacktrace
)

target_compile_options(div0_cli PRIVATE ${DIV0_WARNING_FLAGS})

# ============================================================================
# Main Executable: div0
# ============================================================================
# CLI application with subcommands
# Skip when fuzzing - libFuzzer provides its own main()

if(NOT DIV0_BUILD_FUZZ)
  add_executable(div0 src/cli/main.cpp)

  target_include_directories(div0
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src/cli
      ${CMAKE_CURRENT_BINARY_DIR}/generated
  )

  target_link_libraries(div0
    PRIVATE
      div0_cli
  )

  target_compile_options(div0 PRIVATE ${DIV0_WARNING_FLAGS})

  # Export symbols for readable stack traces in crash handler
  target_link_options(div0 PRIVATE -rdynamic)
endif()

# ============================================================================
# Testing
# ============================================================================

if(DIV0_BUILD_TESTS)
  enable_testing()

  # Configure sanitizer options for tests (leak detection only on Linux)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DIV0_ASAN_OPTIONS "detect_leaks=1:symbolize=1:abort_on_error=1" CACHE STRING "ASan options for tests")
  else()
    set(DIV0_ASAN_OPTIONS "symbolize=1:abort_on_error=1" CACHE STRING "ASan options for tests")
  endif()
  set(DIV0_UBSAN_OPTIONS "print_stacktrace=1:halt_on_error=1" CACHE STRING "UBSan options for tests")

  add_subdirectory(tests)
endif()

# ============================================================================
# Benchmarks
# ============================================================================

if(DIV0_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

# ============================================================================
# Fuzzing
# ============================================================================

if(DIV0_BUILD_FUZZ)
  add_subdirectory(fuzz)
endif()

# ============================================================================
# Build Summary
# ============================================================================

message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "div0 EVM Configuration Summary")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:          ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ standard:      C++${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "  Libraries:")
message(STATUS "    - div0_types     (core types)")
message(STATUS "    - div0_utils     (utilities)")
message(STATUS "    - div0_rlp       (RLP encoding/decoding)")
message(STATUS "    - div0_crypto    (cryptographic primitives)")
message(STATUS "    - div0_trie      (Merkle Patricia Trie)")
message(STATUS "    - div0_ethereum  (Ethereum consensus types)")
message(STATUS "    - div0_db        (key-value database abstraction)")
message(STATUS "    - div0_state     (EVM state management)")
message(STATUS "    - div0_evm       (EVM interpreter)")
message(STATUS "    - div0_cli       (CLI commands)")
message(STATUS "")
message(STATUS "  Build tests:       ${DIV0_BUILD_TESTS}")
message(STATUS "  Build benchmarks:  ${DIV0_BUILD_BENCHMARKS}")
message(STATUS "  Build fuzz:        ${DIV0_BUILD_FUZZ}")
message(STATUS "  Enable LTO:        ${DIV0_ENABLE_LTO}")
message(STATUS "  Enable clang-tidy: ${DIV0_ENABLE_CLANG_TIDY}")
message(STATUS "")
if(DIV0_SANITIZER_BUILD)
  message(STATUS "  Sanitizers:        ENABLED")
endif()
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "")