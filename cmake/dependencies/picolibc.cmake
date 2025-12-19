# picolibc.cmake - picolibc C library for bare-metal builds
#
# Picolibc is a lightweight C library designed for embedded systems.
# Built from source for RISC-V freestanding targets.

include(ExternalProject)

set(PICOLIBC_INSTALL_DIR "${CMAKE_BINARY_DIR}/picolibc-install")

message(STATUS "Configuring picolibc for ${CMAKE_C_COMPILER_TARGET}...")

# Build picolibc
# Note: CMAKE_SYSTEM_PROCESSOR must be "riscv" (not "riscv64") to match picolibc's machine directory
# CMAKE_ASM_COMPILER_TARGET is needed for assembly files to use correct RISC-V syntax
ExternalProject_Add(picolibc_ext
  GIT_REPOSITORY https://github.com/picolibc/picolibc.git
  GIT_TAG        1.8.10
  GIT_SHALLOW    ON
  CMAKE_ARGS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_C_COMPILER_TARGET=${CMAKE_C_COMPILER_TARGET}
    -DCMAKE_ASM_COMPILER_TARGET=${CMAKE_C_COMPILER_TARGET}
    -DCMAKE_SYSTEM_NAME=Generic
    -DCMAKE_SYSTEM_PROCESSOR=riscv
    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
    -DCMAKE_ASM_FLAGS=--target=${CMAKE_C_COMPILER_TARGET}
    -DCMAKE_INSTALL_PREFIX=${PICOLIBC_INSTALL_DIR}
    -DCMAKE_BUILD_TYPE=MinSizeRel
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
    -DPICOLIBC_TLS=OFF
    -DTESTS=OFF
  BUILD_BYPRODUCTS
    ${PICOLIBC_INSTALL_DIR}/lib/libc.a
  INSTALL_DIR ${PICOLIBC_INSTALL_DIR}
)

# Export paths for use by main CMakeLists.txt
set(DIV0_PICOLIBC_INCLUDE_DIR "${PICOLIBC_INSTALL_DIR}/include" CACHE INTERNAL "")
set(DIV0_PICOLIBC_LIB "${PICOLIBC_INSTALL_DIR}/lib/libc.a" CACHE INTERNAL "")