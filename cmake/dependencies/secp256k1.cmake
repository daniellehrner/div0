# secp256k1.cmake - Elliptic curve cryptography library
#
# secp256k1 is Bitcoin Core's optimized ECDSA library, also used by Ethereum.
# Required for transaction signature recovery (ecrecover) and the ECRECOVER
# precompile.
#
# Source: https://github.com/bitcoin-core/secp256k1

set(SECP256K1_GIT_REPO "https://github.com/bitcoin-core/secp256k1.git")
set(SECP256K1_GIT_TAG "v0.7.0")

# Common options
set(SECP256K1_COMMON_OPTIONS
  -DSECP256K1_ENABLE_MODULE_RECOVERY=ON
  -DSECP256K1_BUILD_TESTS=OFF
  -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=OFF
  -DSECP256K1_BUILD_BENCHMARK=OFF
  -DSECP256K1_BUILD_EXAMPLES=OFF
)

# Disable ASM for Debug (ASan compatibility) and freestanding builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR DIV0_FREESTANDING)
  list(APPEND SECP256K1_COMMON_OPTIONS -DSECP256K1_ASM=OFF)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "secp256k1: ASM disabled for Debug build (ASan compatibility)")
  endif()
  if(DIV0_FREESTANDING)
    message(STATUS "secp256k1: ASM disabled for freestanding build")
  endif()
endif()

# ============================================================================
# Freestanding Build (ExternalProject - depends on picolibc)
# ============================================================================
if(DIV0_FREESTANDING)
  include(ExternalProject)

  set(SECP256K1_INSTALL_DIR "${CMAKE_BINARY_DIR}/secp256k1-install")
  set(SECP256K1_LIB_PATH "${SECP256K1_INSTALL_DIR}/lib/libsecp256k1.a")
  set(SECP256K1_INCLUDE_DIR "${SECP256K1_INSTALL_DIR}/include")

  # Create directories at configure time (CMake validates paths early)
  file(MAKE_DIRECTORY ${SECP256K1_INSTALL_DIR}/lib)
  file(MAKE_DIRECTORY ${SECP256K1_INSTALL_DIR}/include)

  # Build secp256k1 with picolibc headers
  set(SECP256K1_C_FLAGS "-ffreestanding -nostdlib -isystem${DIV0_PICOLIBC_INCLUDE_DIR}")

  ExternalProject_Add(secp256k1_ext
    GIT_REPOSITORY ${SECP256K1_GIT_REPO}
    GIT_TAG        ${SECP256K1_GIT_TAG}
    GIT_SHALLOW    ON
    DEPENDS        picolibc_ext
    CMAKE_ARGS
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_C_COMPILER_TARGET=${CMAKE_C_COMPILER_TARGET}
      -DCMAKE_SYSTEM_NAME=Generic
      -DCMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
      "-DCMAKE_C_FLAGS=${SECP256K1_C_FLAGS}"
      -DCMAKE_INSTALL_PREFIX=${SECP256K1_INSTALL_DIR}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
      -DBUILD_SHARED_LIBS=OFF
      ${SECP256K1_COMMON_OPTIONS}
    BUILD_BYPRODUCTS ${SECP256K1_LIB_PATH}
    INSTALL_DIR ${SECP256K1_INSTALL_DIR}
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON
  )

  # Create imported library target
  add_library(secp256k1 STATIC IMPORTED GLOBAL)
  add_dependencies(secp256k1 secp256k1_ext)

  set_target_properties(secp256k1 PROPERTIES
    IMPORTED_LOCATION ${SECP256K1_LIB_PATH}
    INTERFACE_INCLUDE_DIRECTORIES ${SECP256K1_INCLUDE_DIR}
  )

# ============================================================================
# Hosted Build (FetchContent - standard build)
# ============================================================================
else()
  include(FetchContent)

  FetchContent_Declare(
    secp256k1
    GIT_REPOSITORY ${SECP256K1_GIT_REPO}
    GIT_TAG        ${SECP256K1_GIT_TAG}
    GIT_SHALLOW    TRUE
  )

  # Set options before FetchContent_MakeAvailable
  set(SECP256K1_ENABLE_MODULE_RECOVERY ON CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_EXHAUSTIVE_TESTS OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SECP256K1_ASM OFF CACHE STRING "" FORCE)
  endif()

  FetchContent_MakeAvailable(secp256k1)
endif()

message(STATUS "secp256k1: ECDSA signature recovery (ecrecover)")