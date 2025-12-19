# Sanitizers.cmake - Sanitizer configurations for div0
#
# Strategy:
# - Debug builds: ASan + UBSan by default (catch bugs immediately)
# - TSan: Separate build type (cannot be combined with ASan)
#
# This ensures sanitizers run during normal development without
# requiring developers to remember special build types.

# Only configure sanitizers for Clang (best support)
if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(WARNING "Sanitizers are only fully supported with Clang. Current compiler: ${CMAKE_C_COMPILER_ID}")
  return()
endif()

# Skip sanitizers for freestanding builds (not supported)
if(DIV0_FREESTANDING)
  return()
endif()

# ============================================================================
# Debug builds: ASan + UBSan by default
# ============================================================================

set(CMAKE_C_FLAGS_DEBUG
  "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address,undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls"
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
# Cannot be combined with ASan - requires separate build

set(CMAKE_C_FLAGS_THREADSAN
  "-O1 -g -fsanitize=thread -fno-omit-frame-pointer"
  CACHE STRING "Flags used by C compiler during ThreadSan builds" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_THREADSAN
  "-fsanitize=thread"
  CACHE STRING "Flags used by linker during ThreadSan builds" FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_THREADSAN
  "-fsanitize=thread"
  CACHE STRING "Flags used by shared linker during ThreadSan builds" FORCE)

# ============================================================================
# Mark custom build types as valid configurations
# ============================================================================

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;ThreadSan"
  CACHE STRING "Available build types" FORCE)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "Sanitizer configuration:")
message(STATUS "  - Debug     : ASan + UBSan enabled")
message(STATUS "  - Release   : No sanitizers")
message(STATUS "  - ThreadSan : Thread Sanitizer only")