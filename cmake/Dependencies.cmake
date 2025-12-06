# Dependencies.cmake - External dependency management
#
# This module includes all external dependencies used by div0.
# Each dependency has its own file in cmake/dependencies/ for clarity.

set(FETCHCONTENT_QUIET OFF)

# ============================================================================
# Testing & Benchmarking
# ============================================================================

include(dependencies/GTest)

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

include(dependencies/CLI11)
include(dependencies/simdjson)

# ============================================================================
# Debugging
# ============================================================================

include(dependencies/libbacktrace)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "Dependencies configured:")
message(STATUS "  - GTest      : Testing framework")
if(DIV0_BUILD_BENCHMARKS)
  message(STATUS "  - Benchmark  : Performance benchmarking")
endif()
message(STATUS "  - XKCP       : Keccak cryptographic primitives")
message(STATUS "  - secp256k1  : ECDSA signature recovery")
message(STATUS "  - CLI11      : Command-line parser")
message(STATUS "  - simdjson   : JSON parser")
message(STATUS "  - libbacktrace : Stack trace symbolization")
