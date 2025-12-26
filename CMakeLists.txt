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

# Types library (uint256) - no dependencies
add_library(div0_types STATIC
  src/types/uint256.c
)
target_include_directories(div0_types PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
div0_target_options(div0_types)

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

# EVM library (stack, interpreter) - depends on types, mem
add_library(div0_evm STATIC
  src/evm/evm.c
)
target_link_libraries(div0_evm PUBLIC div0_types div0_mem)
div0_target_options(div0_evm)
# Computed goto dispatch tables intentionally override default entries
target_compile_options(div0_evm PRIVATE -Wno-initializer-overrides)

# Convenience interface library linking all components
add_library(div0 INTERFACE)
target_link_libraries(div0 INTERFACE div0_types div0_mem div0_evm)

# ============================================================================
# Tests
# ============================================================================

if(DIV0_BUILD_TESTS)
  enable_testing()

  if(DIV0_FREESTANDING)
    # Freestanding tests - need special linking for QEMU execution
    if(DIV0_FREESTANDING_TESTS AND DIV0_BUILD_PICOLIBC)
      add_executable(div0_tests
        tests/test_div0.c
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
      )

      add_dependencies(div0_tests picolibc_ext compiler_rt_ext)

      target_compile_options(div0_tests PRIVATE ${DIV0_WARNING_FLAGS} -Wno-conversion)
      target_compile_definitions(div0_tests PRIVATE DIV0_FREESTANDING=1)

      # Link statically with picolibc and compiler-rt for QEMU user-mode execution
      # The toolchain file sets CMAKE_C_LINK_EXECUTABLE to wrap everything in --start-group/--end-group
      target_link_libraries(div0_tests PRIVATE
        div0_types
        div0_mem
        div0_evm
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
    # Hosted tests - normal build
    add_executable(div0_tests
      tests/test_div0.c
      ${unity_SOURCE_DIR}/src/unity.c
    )

    target_include_directories(div0_tests PRIVATE
      ${unity_SOURCE_DIR}/src
    )

    target_link_libraries(div0_tests PRIVATE div0)
    div0_target_options(div0_tests)

    add_test(NAME div0_tests COMMAND div0_tests)
  endif()
endif()