# simdjson.cmake - High-performance JSON parser using SIMD
#
# simdjson is the fastest JSON parser, leveraging SIMD instructions
# for parsing. Used by div0 for reading t8n input files (alloc.json,
# env.json, txs.json).
#
# Source: https://github.com/simdjson/simdjson

include(FetchContent)

FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG        v4.2.2
    GIT_SHALLOW    TRUE
)

# Disable developer mode (no tests/examples/tools)
set(SIMDJSON_DEVELOPER_MODE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(simdjson)

message(STATUS "simdjson: SIMD-accelerated JSON parser")