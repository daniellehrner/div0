# spdlog.cmake - Fast C++ logging library
#
# spdlog is a fast, header-only/compiled C++ logging library.
# We use header-only mode for simplicity.
#
# Source: https://github.com/gabime/spdlog

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.16.0
    GIT_SHALLOW    TRUE
)

# Use header-only mode
set(SPDLOG_HEADER_ONLY ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(spdlog)

message(STATUS "spdlog: Fast logging library (header-only)")