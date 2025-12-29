# yyjson.cmake - High-performance JSON library
#
# yyjson is a fast JSON library written in ANSI C with focus on performance.
# Used for t8n (transition tool) JSON parsing/serialization.
#
# Source: https://github.com/ibireme/yyjson
#
# NOTE: This dependency is only included in hosted builds (not freestanding).
# JSON functionality is not needed for zk proofing.

include(FetchContent)

message(STATUS "Fetching yyjson...")
FetchContent_Declare(
  yyjson
  GIT_REPOSITORY https://github.com/ibireme/yyjson.git
  # Pinned to specific commit for supply chain security
  GIT_TAG        8b4a38dc994a110abaec8a400615567bd996105f
  GIT_SHALLOW    OFF  # Specific commit requires full history
  EXCLUDE_FROM_ALL
)

# Disable yyjson features we don't need
set(YYJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YYJSON_BUILD_MISC OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(yyjson)

message(STATUS "yyjson: JSON parsing/serialization for t8n")
