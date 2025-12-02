# CLI11.cmake - Modern command-line argument parser
#
# CLI11 is a header-only C++11/14/17/20 command line parser.
#
# Source: https://github.com/CLIUtils/CLI11

include(FetchContent)

FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.6.1
    GIT_SHALLOW    TRUE
)

# Header-only library - no build configuration needed
FetchContent_MakeAvailable(cli11)

message(STATUS "CLI11: Header-only command-line parser")