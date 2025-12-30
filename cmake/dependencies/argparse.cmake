# argparse.cmake - Command line argument parser
#
# https://github.com/cofyc/argparse

FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/cofyc/argparse.git
  GIT_TAG        4e30aba50340af7f2d3932492c4bffdef7669db8
)
FetchContent_MakeAvailable(argparse)

add_library(argparse STATIC
  ${argparse_SOURCE_DIR}/argparse.c
)
target_include_directories(argparse PUBLIC ${argparse_SOURCE_DIR})

message(STATUS "  - argparse     : Command line argument parser")
