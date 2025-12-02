# Fuzzing

This directory contains [libFuzzer](https://llvm.org/docs/LibFuzzer.html) targets for finding bugs in the EVM implementation.

## What is Fuzzing?

Fuzzing is an automated testing technique that feeds random/mutated inputs to a program to find crashes, memory errors, and undefined behavior. Unlike unit tests that check specific cases, fuzzing explores the input space automatically.

**What fuzzing catches:**
- Buffer overflows (out-of-bounds reads/writes)
- Use-after-free and use-after-scope
- Integer overflows and underflows
- Undefined behavior (shift overflow, null derefs)
- Assertion failures
- Infinite loops (with timeout)

**What fuzzing does NOT catch:**
- Logic errors (wrong output for valid input)
- Performance issues
- Spec compliance (unless combined with differential testing)

## Quick Start

```bash
# Build fuzzers
make fuzz

# Run the EVM fuzzer
./build/fuzz/fuzz/evm_fuzzer corpus/evm/

# Run for a specific duration
./build/fuzz/fuzz/evm_fuzzer corpus/evm/ -max_total_time=300  # 5 minutes

# Run with more parallelism
./build/fuzz/fuzz/evm_fuzzer corpus/evm/ -jobs=4 -workers=4
```

## Understanding the Corpus

The **corpus** is a directory where libFuzzer saves "interesting" inputs - those that increase code coverage.

### How it works:

1. **First run**: Fuzzer starts with empty/random inputs
2. **Discovery**: When an input reaches new code paths, it's saved to the corpus
3. **Mutation**: Fuzzer mutates corpus files to find more interesting inputs
4. **Next run**: Starts from saved corpus, building on previous discoveries

### Corpus management:

```bash
# Create corpus directory
mkdir -p corpus/evm

# Run fuzzer (saves interesting inputs automatically)
./build/fuzz/fuzz/evm_fuzzer corpus/evm/

# Minimize corpus (remove redundant inputs)
mkdir corpus/evm_minimized
./build/fuzz/fuzz/evm_fuzzer -merge=1 corpus/evm_minimized corpus/evm

# Check corpus size
ls corpus/evm | wc -l
```

### Seed corpus:

You can provide initial "seed" inputs to help the fuzzer start with known interesting cases:

```bash
# Add a seed file (e.g., valid EVM bytecode)
echo -ne '\x60\x01\x60\x02\x01\x00' > corpus/evm/add_two_numbers  # PUSH1 1, PUSH1 2, ADD, STOP
```

## Fuzzer Output

```
#12345  NEW    cov: 500 ft: 600 corp: 100/5000b exec/s: 1000 rss: 50Mb
```

| Field | Meaning |
|-------|---------|
| `#12345` | Number of inputs tested |
| `NEW` | This input increased coverage |
| `cov: 500` | Basic block coverage count |
| `ft: 600` | Feature count (more granular than cov) |
| `corp: 100/5000b` | 100 files in corpus, 5000 bytes total |
| `exec/s: 1000` | Executions per second |
| `rss: 50Mb` | Memory usage |

## When a Crash is Found

LibFuzzer saves crashing inputs:

```
==12345==ERROR: AddressSanitizer: heap-buffer-overflow...
artifact_prefix='./'; Test unit written to ./crash-abc123...
```

To reproduce:

```bash
# Run the specific crash input
./build/fuzz/fuzz/evm_fuzzer ./crash-abc123

# Minimize the crash input (find smallest reproducer)
./build/fuzz/fuzz/evm_fuzzer -minimize_crash=1 -runs=10000 ./crash-abc123
```

## Available Fuzzers

| Fuzzer | Description |
|--------|-------------|
| `evm_fuzzer` | Feeds random bytecode to the EVM interpreter |

## Writing a New Fuzzer

Create a new `.cpp` file in `fuzz/`:

```cpp
#include <cstdint>
#include <span>

// libFuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Use data as input to your code
  // Return 0 to accept input into corpus
  // Crashes/sanitizer errors are automatically detected

  return 0;
}
```

The fuzzer will be automatically built by CMake.

## Platform Notes

### macOS

Requires Homebrew LLVM (Apple's clang doesn't include libFuzzer):

```bash
brew install llvm
```

The Makefile automatically uses `/opt/homebrew/opt/llvm/bin/clang++`.

### Linux

System clang includes libFuzzer:

```bash
sudo apt install clang
```

## Resources

- [libFuzzer documentation](https://llvm.org/docs/LibFuzzer.html)
- [Google Fuzzing tutorial](https://github.com/google/fuzzing/blob/master/tutorial/libFuzzerTutorial.md)
- [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)
- [OSS-Fuzz](https://google.github.io/oss-fuzz/)