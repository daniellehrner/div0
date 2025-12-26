# STC.cmake - STC Container Library
#
# Header-only container library with custom allocator support.
# https://github.com/stclib/STC

include(FetchContent)

message(STATUS "Fetching STC...")
FetchContent_Declare(
  stc
  GIT_REPOSITORY https://github.com/stclib/STC.git
  GIT_TAG        v5.0
  GIT_SHALLOW    ON
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(stc)

# STC is header-only - create interface library for include paths
add_library(stc_headers INTERFACE)
target_include_directories(stc_headers INTERFACE
  ${stc_SOURCE_DIR}/include
)