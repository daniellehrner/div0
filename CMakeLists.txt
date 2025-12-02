cmake_minimum_required(VERSION 3.28)
project(div0 VERSION 0.1.0 LANGUAGES CXX)

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
# Library: div0_types
# ============================================================================
# Core types: uint256, addresses, hashes, etc.

add_library(div0_types STATIC
    include/div0/types/address.h
    include/div0/types/bytes32.h
    include/div0/types/storage_slot.h
    include/div0/types/storage_value.h
    include/div0/types/uint256.h
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
# Main Executable: div0
# ============================================================================
# Links all libraries together
# Skip when fuzzing - libFuzzer provides its own main()

if(NOT DIV0_BUILD_FUZZ)
  add_executable(div0 main.cpp)

  target_link_libraries(div0
    PRIVATE
      div0_evm
  )

  target_compile_options(div0 PRIVATE ${DIV0_WARNING_FLAGS})
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
message(STATUS "    - div0_crypto    (cryptographic primitives)")
message(STATUS "    - div0_db        (key-value database abstraction)")
message(STATUS "    - div0_state     (EVM state management)")
message(STATUS "    - div0_evm       (EVM interpreter)")
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