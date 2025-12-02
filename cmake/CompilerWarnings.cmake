# This module sets up strict compiler warnings to catch bugs early.
# Designed for Clang (primary) with GCC compatibility where possible.

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(STATUS "Configuring Clang compiler warnings")

  # Require minimum Clang 15 for proper C++20 support
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0")
    message(FATAL_ERROR "Clang 15+ required for C++20 support (found ${CMAKE_CXX_COMPILER_VERSION})")
  endif()

  set(DIV0_WARNING_FLAGS
    # Base warning sets
    -Wall                    # Enable most warnings
    -Wextra                  # Enable extra warnings
    -Wpedantic               # Strict ISO C++ compliance
    -Werror                  # Treat warnings as errors (enforce quality)

    # Critical for EVM correctness
    -Wconversion             # Implicit type conversions
    -Wsign-conversion        # Sign conversions (critical for uint256 operations)
    -Wshift-overflow         # Shift operations (EVM has many shifts)
    -Wcast-align             # Pointer alignment issues

    # Code quality
    -Wshadow                 # Variable shadowing
    -Wnon-virtual-dtor       # Missing virtual destructors
    -Wold-style-cast         # C-style casts (prefer static_cast, etc.)
    -Wnull-dereference       # Potential null pointer dereferences
    -Wdouble-promotion       # Implicit float to double promotion
    -Wformat=2               # Printf format string checks
    -Wimplicit-fallthrough   # Switch fallthrough without annotation

    # Modern C++ best practices
    -Woverloaded-virtual     # Virtual function hiding
    -Wmisleading-indentation # Misleading indentation (easy to miss bugs)

    # Performance hints
    -Wunused                 # Unused variables, functions, etc.
    -Wunreachable-code       # Dead code detection

    # Clang-specific warnings
    -Wmost                   # Clang's recommended warning set
    -Wloop-analysis          # Loop analysis (potential infinite loops, etc.)
    -Wthread-safety          # Thread safety annotations (for TSan)
    -Wstring-conversion      # String conversion issues

    # Disable some overly pedantic warnings
    -Wno-c++98-compat             # We're using C++20, not C++98
    -Wno-c++98-compat-pedantic    # Same as above
    -Wno-padded                   # Struct padding warnings (too noisy)
    -Wno-unused-parameter         # Common in interfaces, not always avoidable
  )

  message(STATUS "Clang warnings configured: -Wall -Wextra -Wpedantic -Werror + additional checks")

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(STATUS "Configuring GCC compiler warnings")

  set(DIV0_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wnull-dereference
    -Wuseless-cast
    -Wdouble-promotion
    -Wformat=2
  )

  message(STATUS "GCC warnings configured")

else()
  message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Only Clang and GCC are supported.")
endif()

# DIV0_WARNING_FLAGS is available in current scope for use elsewhere