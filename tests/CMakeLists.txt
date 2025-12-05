# tests/CMakeLists.txt - GTest integration for unit and integration tests

include(GoogleTest)

# Common test dependencies
set(TEST_LIBS
  div0_evm
  div0_ethereum
  div0_trie
  div0_crypto
  div0_rlp
  GTest::gtest
  GTest::gtest_main
  GTest::gmock
)

set(TEST_ENV "ASAN_OPTIONS=${DIV0_ASAN_OPTIONS};UBSAN_OPTIONS=${DIV0_UBSAN_OPTIONS}")

# ============================================================================
# Unit Tests (tests/unit/**)
# ============================================================================
file(GLOB_RECURSE UNIT_TEST_SOURCES "unit/*.cpp")

add_executable(div0_unit_tests ${UNIT_TEST_SOURCES})
target_link_libraries(div0_unit_tests PRIVATE ${TEST_LIBS})

gtest_discover_tests(div0_unit_tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTIES
    LABELS "unit"
    ENVIRONMENT "${TEST_ENV}"
)

# ============================================================================
# Integration Tests (tests/integration/**)
# ============================================================================
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES "integration/*.cpp")

add_executable(div0_integration_tests ${INTEGRATION_TEST_SOURCES})
target_link_libraries(div0_integration_tests PRIVATE ${TEST_LIBS})

gtest_discover_tests(div0_integration_tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTIES
    LABELS "integration"
    ENVIRONMENT "${TEST_ENV}"
)

message(STATUS "Unit tests configured with GTest (label: unit)")
message(STATUS "Integration tests configured with GTest (label: integration)")