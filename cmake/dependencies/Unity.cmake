# Unity.cmake - Unity Test Framework
#
# Lightweight C testing framework designed for embedded systems.
# Compiled as part of test executable (not a separate library).

include(FetchContent)

message(STATUS "Fetching Unity...")
FetchContent_Declare(
  unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG        v2.6.1
  GIT_SHALLOW    ON
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(unity)

# Unity source directory is available as ${unity_SOURCE_DIR}