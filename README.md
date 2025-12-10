# div0

A high-performance EVM implementation in C++20.

## Dependencies

### macOS

```bash
brew install cmake ninja ccache llvm lld

# Add Homebrew LLVM's run-clang-tidy to PATH
sudo ln -s /opt/homebrew/opt/llvm/bin/run-clang-tidy /usr/local/bin/run-clang-tidy
```

### Ubuntu 24.04

```bash
# Remove default LLVM 18
sudo apt remove -y clang-18 llvm-18 lld-18

# Install base tools
sudo apt install ninja-build ccache xsltproc wget gpg

# Install CMake 4.x from Kitware APT repository (Ubuntu's default is too old)
test -f /usr/share/doc/kitware-archive-keyring/copyright || \
  wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
  gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main' | \
  sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
sudo apt update
sudo apt install kitware-archive-keyring cmake

# Add LLVM repository (for latest clang/clang-tidy)
LLVM_VERSION=21
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo gpg --dearmor -o /etc/apt/trusted.gpg.d/apt.llvm.org.gpg
sudo add-apt-repository -y "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-${LLVM_VERSION} main"
sudo apt update

# Install LLVM toolchain
sudo apt install clang-${LLVM_VERSION} clang-tidy-${LLVM_VERSION} clang-format-${LLVM_VERSION} \
  clang-tools-${LLVM_VERSION} libc++-${LLVM_VERSION}-dev libc++abi-${LLVM_VERSION}-dev lld-${LLVM_VERSION}

# Create symlinks
sudo ln -sf /usr/bin/clang-${LLVM_VERSION} /usr/bin/clang
sudo ln -sf /usr/bin/clang++-${LLVM_VERSION} /usr/bin/clang++
sudo ln -sf /usr/bin/clang-tidy-${LLVM_VERSION} /usr/bin/clang-tidy
sudo ln -sf /usr/bin/clang-format-${LLVM_VERSION} /usr/bin/clang-format
sudo ln -sf /usr/bin/run-clang-tidy-${LLVM_VERSION} /usr/bin/run-clang-tidy
sudo ln -sf /usr/bin/lld-${LLVM_VERSION} /usr/bin/lld
sudo ln -sf /usr/bin/ld.lld-${LLVM_VERSION} /usr/bin/ld.lld
```

## Build

```bash
make debug      # Debug build with ASan/UBSan
make release    # Optimized build with LTO
make test       # Build and run tests
make bench      # Build and run benchmarks
make fuzz       # Build fuzzers
make clang-tidy # Run static analysis
make format     # Format code with clang-format
make check      # Run format-check + clang-tidy
make clean      # Remove build directories
```

### Test filtering

```bash
# By label (test category)
make test ARGS="-L unit"                 # Run only unit tests
make test ARGS="-L integration"          # Run only integration tests
make test ARGS="-LE integration"         # Exclude integration tests

# By name (regex)
make test ARGS="-R Uint256"              # Run tests matching "Uint256"
make test ARGS="-R 'Division|Overflow'"  # Run tests matching either pattern
make test ARGS="-E Swap"                 # Exclude tests matching "Swap"

# Combined
make test ARGS="-L unit -R Dup"          # Unit tests matching "Dup"
make test ARGS="-R Uint256 -V"           # Verbose output with test details
```

### Benchmark filtering

```bash
make bench ARGS="--benchmark_filter=div"
make bench ARGS="--benchmark_filter='div|mod'"
make bench ARGS="--benchmark_filter=mul --benchmark_repetitions=5"
```

### Fuzzing

```bash
make fuzz                                        # Build fuzzers
./build/fuzz/fuzz/evm_fuzzer corpus/evm/         # Run EVM fuzzer
./build/fuzz/fuzz/evm_fuzzer -max_total_time=60  # Run for 60 seconds
```

See [fuzz/README.md](fuzz/README.md) for detailed fuzzing documentation.

## Execution Spec Tests

The [ethereum/execution-spec-tests](https://github.com/ethereum/execution-spec-tests) verify EVM implementation correctness by running the div0 t8n tool against the official Ethereum test suite.

### Prerequisites

The execution-spec-tests use `uv` for dependency management:

```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

### Running Tests

Clone the execution-spec-tests repository and run tests using `uv run fill`:

```bash
# Clone the test suite (if not already done)
git clone --depth 1 https://github.com/ethereum/execution-spec-tests.git
cd execution-spec-tests

# Run specific tests by keyword filter (-k) and fork (--fork)
uv run fill -k "push0" --fork=Shanghai --evm-bin=/path/to/div0/build/debug/div0

# Run with tracing enabled (for debugging)
uv run fill -k "push0" --fork=Shanghai --evm-bin=./build/debug/div0 --traces

# Run all tests for a specific EIP
uv run fill -k "eip7702" --fork=Prague --evm-bin=./build/release/div0

# Run a specific test file
uv run fill tests/shanghai/eip3855_push0/ --fork=Shanghai --evm-bin=./build/debug/div0

# List available tests without running them
uv run fill --collect-only -k "push0" tests/shanghai/
```

### Key Options

| Option | Description |
|--------|-------------|
| `--evm-bin=PATH` | **Required**: Path to the div0 binary |
| `--fork=FORK` | Target fork (Shanghai, Cancun, Prague, etc.) |
| `-k EXPRESSION` | Filter tests by keyword (pytest style) |
| `--traces` | Enable EVM execution tracing |
| `--clean` | Remove existing fixture files before running |
| `-x` | Stop on first failure |

### Trace Output

When `--traces` is enabled, trace files are created for each transaction:
- Filename format: `trace-{tx_index}-{tx_hash}.jsonl`
- Each line is a JSON object with EIP-3155 fields: `pc`, `op`, `gas`, `gasCost`, `memSize`, `stack`, `depth`, `refund`, `opName`

## CMake Presets

- `debug` - Debug with sanitizers
- `release` - Optimized with LTO
- `relwithdebinfo` - Optimized with debug symbols (profiling)
- `tsan` - Thread sanitizer
- `fuzz` - Fuzzing with libFuzzer

## Debugging

See [docs/core_dumps.md](docs/core_dumps.md) for crash debugging with stack traces and core dumps.
