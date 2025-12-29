# Dependencies.cmake - External dependency management
#
# This module includes all external dependencies used by div0.
# Each dependency has its own file in cmake/dependencies/ for clarity.

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ============================================================================
# Container Library
# ============================================================================

include(dependencies/STC)

# ============================================================================
# Testing
# ============================================================================

if(DIV0_BUILD_TESTS)
  include(dependencies/Unity)
endif()

# ============================================================================
# Freestanding / Bare-metal dependencies
# ============================================================================

if(DIV0_FREESTANDING AND DIV0_BUILD_PICOLIBC)
  include(dependencies/picolibc)
  include(dependencies/compiler-rt)
endif()

# ============================================================================
# Cryptography
# ============================================================================

include(dependencies/XKCP)
include(dependencies/secp256k1)

# ============================================================================
# JSON Library (hosted builds only)
# ============================================================================

if(NOT DIV0_FREESTANDING)
  include(dependencies/yyjson)
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "Dependencies configured:")
message(STATUS "  - STC          : Container library (header-only)")
message(STATUS "  - XKCP         : Keccak-256 hash function")
message(STATUS "  - secp256k1    : ECDSA signature recovery")
if(DIV0_BUILD_TESTS)
  message(STATUS "  - Unity        : C testing framework")
endif()
if(DIV0_FREESTANDING AND DIV0_BUILD_PICOLIBC)
  message(STATUS "  - picolibc     : Embedded C library")
  message(STATUS "  - compiler-rt  : LLVM runtime builtins")
endif()
if(NOT DIV0_FREESTANDING)
  message(STATUS "  - yyjson       : JSON parsing/serialization")
endif()
