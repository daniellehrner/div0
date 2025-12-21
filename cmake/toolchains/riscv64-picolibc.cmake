# RISC-V 64-bit bare-metal cross-compilation toolchain for Clang with picolibc
#
# Usage:
#   cmake -B build-riscv \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-picolibc.cmake \
#     -DPICOLIBC_ROOT=/path/to/picolibc/riscv64-unknown-elf
#
# Or set PICOLIBC_ROOT environment variable before running cmake.
#

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# Clang cross-compiler settings
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET riscv64-unknown-elf)

# Don't try to compile test programs during toolchain detection
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# picolibc installation path - check cache, then environment
if(NOT DEFINED PICOLIBC_ROOT)
    if(DEFINED ENV{PICOLIBC_ROOT})
        set(PICOLIBC_ROOT "$ENV{PICOLIBC_ROOT}" CACHE PATH "picolibc installation root")
    endif()
endif()

# Enable picolibc build option (for tests)
set(DIV0_BUILD_PICOLIBC ON CACHE BOOL "Use picolibc for freestanding builds")

# Only set picolibc-specific flags if PICOLIBC_ROOT is defined and valid
# (skipped during try_compile when it's not passed through)
if(DEFINED PICOLIBC_ROOT AND EXISTS "${PICOLIBC_ROOT}/include")
    # RISC-V specific flags
    # - rv64imac: 64-bit base integer + multiply + atomics + compressed
    # - lp64: 64-bit long and pointers (no floating point in ABI)
    # - medany: medium code model for any address
    set(RISCV_FLAGS "-march=rv64imac -mabi=lp64 -mcmodel=medany")

    # Freestanding and picolibc flags
    # Use smaller arena block size (4KB) for bare-metal to reduce memory pressure
    set(FREESTANDING_FLAGS "-ffreestanding -fno-builtin -DDIV0_ARENA_BLOCK_SIZE=4096")

    # picolibc include path
    set(PICOLIBC_INCLUDE_FLAGS "-isystem ${PICOLIBC_ROOT}/include")

    # Combine all C flags
    set(CMAKE_C_FLAGS_INIT "${RISCV_FLAGS} ${FREESTANDING_FLAGS} ${PICOLIBC_INCLUDE_FLAGS}")

    # Linker flags - link against picolibc
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${PICOLIBC_ROOT}/lib -lc -lm -nostartfiles")

    # Search paths for find_* commands
    set(CMAKE_FIND_ROOT_PATH ${PICOLIBC_ROOT})
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
