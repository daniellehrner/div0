# fuzz/CMakeLists.txt - libFuzzer integration

# Ensure we're building with Fuzz configuration
if(NOT CMAKE_BUILD_TYPE STREQUAL "Fuzz")
  message(FATAL_ERROR "Fuzzing targets must be built with -DCMAKE_BUILD_TYPE=Fuzz "
                      "or using the 'fuzz' preset: cmake --preset=fuzz")
endif()

# Find all fuzzer source files
file(GLOB FUZZER_SOURCES "*.cpp")

# Create a separate executable for each fuzzer
# Each fuzzer should implement: extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
foreach(FUZZER_SOURCE ${FUZZER_SOURCES})
  # Get filename without extension
  get_filename_component(FUZZER_NAME ${FUZZER_SOURCE} NAME_WE)

  # Create fuzzer executable
  add_executable(${FUZZER_NAME} ${FUZZER_SOURCE})

  # Link against the libraries being fuzzed
  target_link_libraries(${FUZZER_NAME}
    PRIVATE
      div0_types
      div0_evm
  )

  message(STATUS "Fuzzer configured: ${FUZZER_NAME}")
endforeach()

message(STATUS "")
message(STATUS "Fuzzing targets built with libFuzzer + ASan")
message(STATUS "Run a fuzzer: ./build/fuzz/<fuzzer_name> [corpus_dir]")
message(STATUS "")