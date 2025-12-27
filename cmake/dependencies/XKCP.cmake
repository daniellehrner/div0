# XKCP.cmake - eXtended Keccak Code Package integration
#
# XKCP provides optimized Keccak implementations for different CPU architectures.
# It uses a non-standard build system (xsltproc + make), so we use ExternalProject
# to invoke its native build and then import the resulting static library.
#
# Minimum CPU requirements (compile-time selection):
#   x86_64: AVX2 (Intel Haswell 2013+, AMD Excavator 2015+)
#   ARM64:  ARMv8A (includes NEON - standard on all ARM64)
#   Other:  generic64 (portable C fallback)

include(ExternalProject)
include(FetchContent)

# ============================================================================
# Architecture Selection
# ============================================================================
# Compile-time selection based on target architecture.
# No runtime dispatch - we require minimum CPU features.

function(select_xkcp_target OUT_TARGET)
  # Force generic64 (portable C) for freestanding builds
  # This avoids assembly dependencies that may not work on all platforms
  if(DIV0_FREESTANDING)
    set(${OUT_TARGET} "generic64" PARENT_SCOPE)
    message(STATUS "XKCP: Freestanding mode, using generic64 (portable C)")
    return()
  endif()

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    # x86_64: Use AVX2 (available since 2013)
    set(${OUT_TARGET} "AVX2" PARENT_SCOPE)
    message(STATUS "XKCP: x86_64 detected, using AVX2 target")

  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    # ARM64: Use ARMv8A which includes NEON
    set(${OUT_TARGET} "ARMv8A" PARENT_SCOPE)
    message(STATUS "XKCP: ARM64 detected, using ARMv8A target")

  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64|riscv32")
    # RISC-V: Use generic64 (no optimized XKCP target available)
    set(${OUT_TARGET} "generic64" PARENT_SCOPE)
    message(STATUS "XKCP: RISC-V detected, using generic64 (portable C)")

  else()
    # Fallback for unknown architectures
    set(${OUT_TARGET} "generic64" PARENT_SCOPE)
    message(WARNING "XKCP: Unknown architecture '${CMAKE_SYSTEM_PROCESSOR}', using generic64")
  endif()
endfunction()

# ============================================================================
# Select Target
# ============================================================================

select_xkcp_target(XKCP_TARGET)

# ============================================================================
# Fetch XKCP Source
# ============================================================================

set(XKCP_GIT_REPO "https://github.com/XKCP/XKCP.git")
set(XKCP_GIT_TAG "716f007dd73ef28d357b8162173646be574ad1b7")

FetchContent_Declare(
  xkcp_src
  GIT_REPOSITORY ${XKCP_GIT_REPO}
  GIT_TAG ${XKCP_GIT_TAG}
  GIT_SHALLOW ON
)

message(STATUS "Fetching XKCP source...")
FetchContent_MakeAvailable(xkcp_src)

# ============================================================================
# Build XKCP via ExternalProject
# ============================================================================
# XKCP's build system creates targets like "AVX2/libXKCP.a" in bin/
# We build it in-source and copy the results to our install directory.

set(XKCP_INSTALL_DIR "${CMAKE_BINARY_DIR}/xkcp-install")
set(XKCP_LIB_PATH "${XKCP_INSTALL_DIR}/lib/libXKCP.a")
set(XKCP_INCLUDE_DIR "${XKCP_INSTALL_DIR}/include")

# Create directories now - CMake validates INTERFACE_INCLUDE_DIRECTORIES at configure time
# but ExternalProject runs at build time
file(MAKE_DIRECTORY ${XKCP_INSTALL_DIR}/lib)
file(MAKE_DIRECTORY ${XKCP_INSTALL_DIR}/include)

# Set up cross-compilation for XKCP
if(DIV0_FREESTANDING AND CMAKE_C_COMPILER_TARGET)
  # Cross-compile: pass target to clang via environment
  # XKCP's makefile respects CC and CFLAGS environment variables
  set(XKCP_CC_VALUE "${CMAKE_C_COMPILER} --target=${CMAKE_C_COMPILER_TARGET}")
  set(XKCP_CFLAGS_VALUE "-ffreestanding -fno-builtin -isystem${DIV0_PICOLIBC_INCLUDE_DIR}")
  set(XKCP_ENV ${CMAKE_COMMAND} -E env "CC=${XKCP_CC_VALUE}" "CFLAGS=${XKCP_CFLAGS_VALUE}")
  set(XKCP_BUILD_CMD ${XKCP_ENV} make ${XKCP_TARGET}/libXKCP.a)
  set(XKCP_DEPENDS picolibc_ext)
else()
  # Native build
  set(XKCP_BUILD_CMD make ${XKCP_TARGET}/libXKCP.a)
  set(XKCP_DEPENDS "")
endif()

ExternalProject_Add(xkcp_build
  SOURCE_DIR ${xkcp_src_SOURCE_DIR}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${XKCP_BUILD_CMD}
  BUILD_IN_SOURCE TRUE
  DEPENDS ${XKCP_DEPENDS}
  # Multiple COMMAND entries for install step
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
    ${xkcp_src_SOURCE_DIR}/bin/${XKCP_TARGET}/libXKCP.a
    ${XKCP_INSTALL_DIR}/lib/
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${xkcp_src_SOURCE_DIR}/bin/${XKCP_TARGET}/libXKCP.a.headers
    ${XKCP_INCLUDE_DIR}
  BUILD_BYPRODUCTS ${XKCP_LIB_PATH}
  LOG_BUILD ON
  LOG_INSTALL ON
)

# ============================================================================
# Create Imported Library Target
# ============================================================================
# This allows other targets to link against XKCP using target_link_libraries()

add_library(xkcp STATIC IMPORTED GLOBAL)
add_dependencies(xkcp xkcp_build)

set_target_properties(xkcp PROPERTIES
  IMPORTED_LOCATION ${XKCP_LIB_PATH}
  INTERFACE_INCLUDE_DIRECTORIES ${XKCP_INCLUDE_DIR}
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "XKCP Configuration:")
message(STATUS "  - Target:     ${XKCP_TARGET}")
message(STATUS "  - Library:    ${XKCP_LIB_PATH}")
message(STATUS "  - Headers:    ${XKCP_INCLUDE_DIR}")