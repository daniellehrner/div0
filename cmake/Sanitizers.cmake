# Sanitizers.cmake - Sanitizer configurations
#
# Strategy:
# - Debug builds: Include ASan + UBSan by default (catch bugs immediately)
# - TSan: Separate build type (cannot be combined with ASan)
# - Fuzz: Separate build type (libFuzzer with ASan)
#
# This approach ensures sanitizers run during normal development without
# requiring developers to remember to use special build types.

# Only configure sanitizers for Clang (best support)
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(WARNING "Sanitizers are only fully supported with Clang. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
  return()
endif()

# ============================================================================
# Debug builds: Include ASan + UBSan by default
# ============================================================================
# This is the recommended approach for security-critical code.
# Catches memory errors and undefined behavior during normal development.

set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-sanitize-recover=undefined"
  CACHE STRING "Flags used by C++ compiler during Debug builds" FORCE)

set(CMAKE_C_FLAGS_DEBUG
  "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address,undefined -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-sanitize-recover=undefined"
  CACHE STRING "Flags used by C compiler during Debug builds" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEBUG
  "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address,undefined"
  CACHE STRING "Flags used by linker during Debug builds" FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_DEBUG
  "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address,undefined"
  CACHE STRING "Flags used by shared linker during Debug builds" FORCE)

# ============================================================================
# TSan (Thread Sanitizer) - Separate build type
# ============================================================================
# Catches: data races, deadlocks, incorrect thread API usage
# Overhead: ~5-15x slowdown, ~5-10x memory
# NOTE: Cannot be combined with ASan - requires separate build

set(CMAKE_C_FLAGS_TSAN
  "-O1 -g -fsanitize=thread -fno-omit-frame-pointer"
  CACHE STRING "Flags used by C compiler during TSan builds" FORCE)

set(CMAKE_CXX_FLAGS_TSAN
  "-O1 -g -fsanitize=thread -fno-omit-frame-pointer"
  CACHE STRING "Flags used by C++ compiler during TSan builds" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_TSAN
  "-fsanitize=thread"
  CACHE STRING "Flags used by linker during TSan builds" FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_TSAN
  "-fsanitize=thread"
  CACHE STRING "Flags used by shared linker during TSan builds" FORCE)

# ============================================================================
# Fuzzing build type (libFuzzer with ASan)
# ============================================================================
# Uses libFuzzer for coverage-guided fuzzing
# Combined with ASan to catch memory bugs during fuzzing
#
# Note on macOS with Homebrew LLVM:
# - libFuzzer runtime is built against Homebrew's libc++
# - Must use Homebrew's libc++ for ABI compatibility (not Apple's from SDK)
# - System headers from SDK are still needed via -isysroot for POSIX APIs

set(FUZZ_COMPILE_FLAGS "")
set(FUZZ_LINK_FLAGS "")
if(APPLE)
  execute_process(
    COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE MACOS_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(MACOS_SDK_PATH)
    # Use SDK for system headers, but Homebrew's libc++ for ABI compatibility
    set(FUZZ_COMPILE_FLAGS "-isysroot ${MACOS_SDK_PATH} -I/opt/homebrew/opt/llvm/include/c++/v1")
    set(FUZZ_LINK_FLAGS "-L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++")
  endif()
endif()

set(CMAKE_C_FLAGS_FUZZ
  "-O1 -g -fsanitize=fuzzer,address -fno-omit-frame-pointer ${FUZZ_COMPILE_FLAGS}"
  CACHE STRING "Flags used by C compiler during Fuzz builds" FORCE)

set(CMAKE_CXX_FLAGS_FUZZ
  "-O1 -g -fsanitize=fuzzer,address -fno-omit-frame-pointer ${FUZZ_COMPILE_FLAGS}"
  CACHE STRING "Flags used by C++ compiler during Fuzz builds" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_FUZZ
  "-fsanitize=fuzzer,address ${FUZZ_LINK_FLAGS}"
  CACHE STRING "Flags used by linker during Fuzz builds" FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_FUZZ
  "-fsanitize=fuzzer,address ${FUZZ_LINK_FLAGS}"
  CACHE STRING "Flags used by shared linker during Fuzz builds" FORCE)

# ============================================================================
# Mark custom build types as valid configurations
# ============================================================================

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;TSan;Fuzz"
  CACHE STRING "Available build types" FORCE)

# ============================================================================
# Sanitizer runtime options and information
# ============================================================================

message(STATUS "Sanitizer configuration:")
message(STATUS "  - Debug   : ASan + UBSan enabled by default (safe development)")
message(STATUS "  - Release : No sanitizers (maximum performance)")
message(STATUS "  - TSan    : Thread Sanitizer only (separate build)")
message(STATUS "  - Fuzz    : libFuzzer with ASan")
message(STATUS "")
message(STATUS "Recommended runtime options:")
message(STATUS "  Debug/Fuzz: export ASAN_OPTIONS='detect_leaks=1:symbolize=1:abort_on_error=1'")
message(STATUS "  Debug/Fuzz: export UBSAN_OPTIONS='print_stacktrace=1:halt_on_error=1'")
message(STATUS "  TSan      : export TSAN_OPTIONS='second_deadlock_stack=1'")

# ============================================================================
# Helper function to check if a sanitizer build is active
# ============================================================================

function(div0_is_sanitizer_build result_var)
  if(CMAKE_BUILD_TYPE MATCHES "Debug|TSan|Fuzz")
    set(${result_var} TRUE PARENT_SCOPE)
  else()
    set(${result_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

# Set DIV0_SANITIZER_BUILD variable for use in main CMakeLists.txt
div0_is_sanitizer_build(DIV0_SANITIZER_BUILD)