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

# Use add_compile_options/add_link_options to append flags without overwriting
# user-specified CMAKE_C_FLAGS_DEBUG. Generator expressions ensure these only
# apply to Debug builds.
add_compile_options(
  $<$<CONFIG:Debug>:-g>
  $<$<CONFIG:Debug>:-fsanitize=address,undefined>
  $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
  $<$<CONFIG:Debug>:-fno-optimize-sibling-calls>
)

add_link_options(
  $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# ============================================================================
# TSan (Thread Sanitizer) - Separate build type
# ============================================================================
# Cannot be combined with ASan - requires separate build

add_compile_options(
  $<$<CONFIG:ThreadSan>:-O1>
  $<$<CONFIG:ThreadSan>:-g>
  $<$<CONFIG:ThreadSan>:-fsanitize=thread>
  $<$<CONFIG:ThreadSan>:-fno-omit-frame-pointer>
)

add_link_options(
  $<$<CONFIG:ThreadSan>:-fsanitize=thread>
)

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