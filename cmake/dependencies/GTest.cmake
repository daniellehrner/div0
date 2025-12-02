# GTest.cmake - Google Test framework
#
# Industry-standard C++ testing framework
# Used for unit tests and integration tests

include(FetchContent)

message(STATUS "Fetching GTest...")
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
  GIT_SHALLOW    ON
)

# Prevent GTest from overriding compiler settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "Build GMock" FORCE)

FetchContent_MakeAvailable(googletest)
