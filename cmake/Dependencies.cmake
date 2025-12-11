# Dependencies.cmake - External dependency management
#
# This module includes all external dependencies used by div0.
# Each dependency has its own file in cmake/dependencies/ for clarity.

set(FETCHCONTENT_QUIET OFF)

# ============================================================================
# Testing & Benchmarking
# ============================================================================

if(DIV0_BUILD_TESTS)
  include(dependencies/GTest)
endif()

if(DIV0_BUILD_BENCHMARKS)
  include(dependencies/Benchmark)
endif()

# ============================================================================
# Cryptography
# ============================================================================

include(dependencies/XKCP)
include(dependencies/secp256k1)

# ============================================================================
# CLI & Serialization
# ============================================================================

if(DIV0_BUILD_CLI)
  include(dependencies/CLI11)
  include(dependencies/spdlog)
  include(dependencies/libbacktrace)
endif()

# JSON parsing - not needed for bare-metal builds
if(NOT DIV0_BARE_METAL)
  include(dependencies/simdjson)
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "Dependencies configured:")
if(DIV0_BUILD_TESTS)
  message(STATUS "  - GTest      : Testing framework")
endif()
if(DIV0_BUILD_BENCHMARKS)
  message(STATUS "  - Benchmark  : Performance benchmarking")
endif()
message(STATUS "  - XKCP       : Keccak cryptographic primitives")
message(STATUS "  - secp256k1  : ECDSA signature recovery")
if(DIV0_BUILD_CLI)
  message(STATUS "  - CLI11      : Command-line parser")
  message(STATUS "  - spdlog     : Logging library")
  message(STATUS "  - libbacktrace : Stack trace symbolization")
endif()
if(NOT DIV0_BARE_METAL)
  message(STATUS "  - simdjson   : JSON parser")
endif()
