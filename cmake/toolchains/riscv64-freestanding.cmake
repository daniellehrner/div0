# RISC-V 64-bit bare-metal cross-compilation toolchain for Clang
#
# This toolchain builds picolibc from source via ExternalProject.
# No external dependencies required - everything is fetched and built.
#
# Usage:
#   cmake --preset riscv64
#

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# Clang cross-compiler settings
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET riscv64-unknown-elf)

# RISC-V specific flags
# - rv64imac: 64-bit base integer + multiply + atomics + compressed
# - lp64: 64-bit long and pointers (no floating point in ABI)
# - medany: medium code model for any address
set(RISCV_ARCH_FLAGS "-march=rv64imac -mabi=lp64 -mcmodel=medany")

# Freestanding flags (picolibc paths added later by main CMakeLists.txt)
set(CMAKE_C_FLAGS_INIT "${RISCV_ARCH_FLAGS} -ffreestanding -fno-builtin")

# Don't try to compile test programs during toolchain detection
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Mark that we want picolibc built from source
set(DIV0_BUILD_PICOLIBC ON CACHE BOOL "Build picolibc from source")

# Search paths - will be populated after picolibc is built
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Override executable link command to wrap ALL inputs in --start-group/--end-group
# This is necessary for freestanding builds where object files depend on libc
# which may have internal dependencies requiring multiple passes
set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> -Wl,--start-group <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group -o <TARGET>"
)