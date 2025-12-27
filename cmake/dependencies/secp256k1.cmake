# secp256k1.cmake - Elliptic curve cryptography library
#
# secp256k1 is Bitcoin Core's optimized ECDSA library, also used by Ethereum.
# Required for transaction signature recovery (ecrecover) and the ECRECOVER
# precompile.
#
# Source: https://github.com/bitcoin-core/secp256k1

include(FetchContent)

FetchContent_Declare(
  secp256k1
  GIT_REPOSITORY https://github.com/bitcoin-core/secp256k1.git
  GIT_TAG        v0.7.0
  GIT_SHALLOW    TRUE
)

# Enable recovery module (required for ecrecover)
set(SECP256K1_ENABLE_MODULE_RECOVERY ON CACHE BOOL "" FORCE)

# Disable features we don't need
set(SECP256K1_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_EXHAUSTIVE_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Disable ASM when sanitizers are enabled - ASan uses extra registers that
# conflict with secp256k1's inline assembly.
# Also disable ASM for freestanding builds to ensure portability.
# Debug builds have ASan enabled by default in this project.
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR DIV0_FREESTANDING)
  set(SECP256K1_ASM OFF CACHE STRING "" FORCE)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "secp256k1: ASM disabled for Debug build (ASan compatibility)")
  endif()
  if(DIV0_FREESTANDING)
    message(STATUS "secp256k1: ASM disabled for freestanding build")
  endif()
endif()

FetchContent_MakeAvailable(secp256k1)

message(STATUS "secp256k1: ECDSA signature recovery (ecrecover)")