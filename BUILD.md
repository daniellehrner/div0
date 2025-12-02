# Building div0

This document describes how to build and test the div0 Ethereum EVM implementation.

## Prerequisites

### Required

- **CMake** 3.28 or later
- **Clang** 15 or later (primary compiler)
- **Ninja** build system (recommended)
- **ccache** (optional, for faster rebuilds)

### macOS

```bash
brew install cmake ninja ccache llvm
```

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install cmake ninja-build ccache clang-15 libc++-15-dev libc++abi-15-dev
```

## Build Configurations

The project uses CMake presets for different build configurations:

- **debug** - Development build with ASan + UBSan enabled (default)
- **release** - Optimized build with LTO
- **relwithdebinfo** - Optimized build with debug symbols (for profiling)
- **tsan** - Thread Sanitizer build (for concurrency testing)
- **fuzz** - Fuzzing build with libFuzzer + ASan

## Quick Start

### Debug Build (Recommended for Development)

```bash
# Configure
cmake --preset=debug

# Build
cmake --build --preset=debug

# Run tests
ctest --preset=debug

# Run the executable
./build/debug/div0
```

The debug build includes **ASan** (Address Sanitizer) and **UBSan** (Undefined Behavior Sanitizer) by default, catching bugs early during development.

### Release Build (For Performance)

```bash
# Configure
cmake --preset=release

# Build
cmake --build --preset=release

# Run tests
ctest --preset=release

# Run benchmarks
./build/release/div0_bench

# Run the executable
./build/release/div0
```

## Testing

### Running Unit Tests

```bash
# With debug build (sanitizers enabled)
cmake --preset=debug
cmake --build --preset=debug
ctest --preset=debug --output-on-failure

# With release build
cmake --preset=release
cmake --build --preset=release
ctest --preset=release --output-on-failure
```

### Thread Sanitizer (TSan)

For testing concurrent code:

```bash
cmake --preset=tsan
cmake --build --preset=tsan
ctest --preset=tsan --output-on-failure
```

**Note:** TSan cannot be combined with ASan, so it's a separate build.

## Benchmarking

Benchmarks should only be run in Release or RelWithDebInfo builds:

```bash
cmake --preset=release
cmake --build --preset=release

# Run all benchmarks
./build/release/benchmarks/div0_bench

# Run specific benchmark with filters
./build/release/benchmarks/div0_bench --benchmark_filter=Opcode.*

# Output as JSON
./build/release/benchmarks/div0_bench --benchmark_format=json --benchmark_out=results.json
```

## Fuzzing

Fuzzing uses libFuzzer with ASan:

```bash
# Configure and build
cmake --preset=fuzz
cmake --build --preset=fuzz

# Run a fuzzer (replace <fuzzer_name> with actual fuzzer)
./build/fuzz/<fuzzer_name>

## Sanitizer Runtime Options

### ASan (Address Sanitizer)

Set environment variables for better output:

```bash
export ASAN_OPTIONS='detect_leaks=1:symbolize=1:abort_on_error=1'
```

### UBSan (Undefined Behavior Sanitizer)

```bash
export UBSAN_OPTIONS='print_stacktrace=1:halt_on_error=1'
```

### TSan (Thread Sanitizer)

```bash
export TSAN_OPTIONS='second_deadlock_stack=1'
```

## Static Analysis

### clang-tidy

clang-tidy runs automatically during compilation (can be disabled with `-DDIV0_ENABLE_CLANG_TIDY=OFF`).

To run manually:

```bash
cmake --preset=debug
clang-tidy -p build/debug src/**/*.cpp
```

## Build Options

You can override default options:

```bash
# Disable clang-tidy
cmake --preset=debug -DDIV0_ENABLE_CLANG_TIDY=OFF

# Disable LTO in release build
cmake --preset=release -DDIV0_ENABLE_LTO=OFF

# Disable tests
cmake --preset=debug -DDIV0_BUILD_TESTS=OFF

# Enable benchmarks in debug (not recommended)
cmake --preset=debug -DDIV0_BUILD_BENCHMARKS=ON
```

## Cleaning Builds

```bash
# Remove all build directories
rm -rf build/

# Remove CMake cache for specific preset
rm -rf build/debug/
cmake --preset=debug
```

## Troubleshooting

### "Sanitizers are only fully supported with Clang"

Ensure you're using Clang as your compiler:

```bash
export CC=clang
export CXX=clang++
cmake --preset=debug
```

### "ccache not found"

Install ccache or configure CMake to not use it:

```bash
# Edit CMakeLists.txt and comment out ccache lines, or:
cmake --preset=debug -DCMAKE_C_COMPILER_LAUNCHER="" -DCMAKE_CXX_COMPILER_LAUNCHER=""
```

### Link errors with libc++

On Linux, ensure you have libc++ installed:

```bash
# Ubuntu/Debian
sudo apt install libc++-dev libc++abi-dev

# Arch
sudo pacman -S libc++
```