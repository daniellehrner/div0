# CompilerWarnings.cmake - Strict compiler warnings for div0
#
# Designed for Clang (primary) with GCC compatibility.
# Uses C23 standard with GNU extensions (for computed gotos).

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(STATUS "Configuring Clang compiler warnings")

  set(DIV0_WARNING_FLAGS
    # Base warning sets
    -Wall
    -Wextra
    -Werror

    # Critical for EVM correctness
    -Wconversion
    -Wsign-conversion
    -Wshift-overflow
    -Wcast-align

    # Code quality
    -Wshadow
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough

    # Performance hints
    -Wunused
    -Wunreachable-code

    # Note: -Wpedantic removed because we use GNU extensions (computed gotos)
  )

  message(STATUS "Clang warnings configured: -Wall -Wextra -Werror + additional checks")

elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  message(STATUS "Configuring GCC compiler warnings")

  set(DIV0_WARNING_FLAGS
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wcast-align
    -Wunused
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
  )

  message(STATUS "GCC warnings configured")

else()
  message(WARNING "Unsupported compiler: ${CMAKE_C_COMPILER_ID}. Only Clang and GCC are supported.")
  set(DIV0_WARNING_FLAGS -Wall -Wextra)
endif()

# DIV0_WARNING_FLAGS is available for use in main CMakeLists.txt