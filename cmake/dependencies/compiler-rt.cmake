# compiler-rt.cmake - LLVM compiler-rt builtins for bare-metal builds
#
# Provides runtime support functions (soft-float, 128-bit integer ops, etc.)
# for targets without hardware support.

include(ExternalProject)

set(COMPILER_RT_BUILD_DIR "${CMAKE_BINARY_DIR}/compiler_rt_ext-prefix/src/compiler_rt_ext-build")

message(STATUS "Configuring compiler-rt builtins for ${CMAKE_C_COMPILER_TARGET}...")

ExternalProject_Add(compiler_rt_ext
  GIT_REPOSITORY https://github.com/llvm/llvm-project.git
  GIT_TAG        llvmorg-18.1.8
  GIT_SHALLOW    ON
  SOURCE_SUBDIR  compiler-rt
  CMAKE_ARGS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_C_COMPILER_TARGET=${CMAKE_C_COMPILER_TARGET}
    -DCMAKE_ASM_COMPILER_TARGET=${CMAKE_C_COMPILER_TARGET}
    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
    -DCOMPILER_RT_BUILD_BUILTINS=ON
    -DCOMPILER_RT_BUILD_SANITIZERS=OFF
    -DCOMPILER_RT_BUILD_XRAY=OFF
    -DCOMPILER_RT_BUILD_LIBFUZZER=OFF
    -DCOMPILER_RT_BUILD_PROFILE=OFF
    -DCOMPILER_RT_BUILD_MEMPROF=OFF
    -DCOMPILER_RT_BUILD_ORC=OFF
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON
    -DCOMPILER_RT_BAREMETAL_BUILD=ON
    -DLLVM_CONFIG_PATH=""
  BUILD_BYPRODUCTS
    ${COMPILER_RT_BUILD_DIR}/lib/linux/libclang_rt.builtins-riscv64.a
  INSTALL_COMMAND ""
)

# Export path for use by main CMakeLists.txt
set(DIV0_COMPILER_RT_LIB "${COMPILER_RT_BUILD_DIR}/lib/linux/libclang_rt.builtins-riscv64.a" CACHE INTERNAL "")