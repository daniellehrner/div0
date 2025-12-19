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

# picolibc installation path - check cache, then environment, then fail with helpful message
if(NOT DEFINED PICOLIBC_ROOT)
    if(DEFINED ENV{PICOLIBC_ROOT})
        set(PICOLIBC_ROOT "$ENV{PICOLIBC_ROOT}" CACHE PATH "picolibc installation root")
    else()
        message(FATAL_ERROR
            "PICOLIBC_ROOT not set. Please specify the picolibc installation path:\n"
            "  cmake -DPICOLIBC_ROOT=/path/to/picolibc/riscv64-unknown-elf ...\n"
            "Or set the PICOLIBC_ROOT environment variable.\n"
            "Common locations:\n"
            "  Linux: /usr/lib/picolibc/riscv64-unknown-elf\n"
            "  macOS (Homebrew): /opt/homebrew/opt/picolibc/riscv64-unknown-elf"
        )
    endif()
endif()

# Verify picolibc installation
if(NOT EXISTS "${PICOLIBC_ROOT}/include")
    message(FATAL_ERROR "picolibc include directory not found at: ${PICOLIBC_ROOT}/include")
endif()

# RISC-V specific flags
# - rv64imac: 64-bit base integer + multiply + atomics + compressed
# - lp64: 64-bit long and pointers (no floating point in ABI)
# - medany: medium code model for any address
set(RISCV_FLAGS "-march=rv64imac -mabi=lp64 -mcmodel=medany")

# Freestanding and picolibc flags
set(FREESTANDING_FLAGS "-ffreestanding -fno-builtin")

# picolibc include path
set(PICOLIBC_INCLUDE_FLAGS "-isystem ${PICOLIBC_ROOT}/include")

# Combine all C flags
set(CMAKE_C_FLAGS_INIT "${RISCV_FLAGS} ${FREESTANDING_FLAGS} ${PICOLIBC_INCLUDE_FLAGS}")

# Linker flags - link against picolibc
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${PICOLIBC_ROOT}/lib -lc -lm -nostartfiles")

# Don't try to compile test programs during toolchain detection
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search paths for find_* commands
set(CMAKE_FIND_ROOT_PATH ${PICOLIBC_ROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)