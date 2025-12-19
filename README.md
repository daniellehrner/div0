# div0

High-performance EVM (Ethereum Virtual Machine) implementation in C23.

## Features

- Written in modern C23
- Multi-platform: x86_64, ARM64, RISC-V bare-metal
- Memory-safe with ASan/UBSan in debug builds
- Thread-safe with TSan verification

## Dependencies

### macOS

```bash
brew install cmake ninja ccache llvm lld
```

### Ubuntu 24.04

```bash
# Install base tools
sudo apt install ninja-build ccache wget gpg

# Install CMake 4.x from Kitware APT repository (Ubuntu's default is too old)
test -f /usr/share/doc/kitware-archive-keyring/copyright || \
  wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
  gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main' | \
  sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
sudo apt update
sudo apt install kitware-archive-keyring cmake

# Add LLVM repository (for latest clang)
LLVM_VERSION=21
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo gpg --dearmor -o /etc/apt/trusted.gpg.d/apt.llvm.org.gpg
sudo add-apt-repository -y "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-${LLVM_VERSION} main"
sudo apt update

# Install LLVM toolchain
sudo apt install clang-${LLVM_VERSION} clang-tidy-${LLVM_VERSION} clang-format-${LLVM_VERSION} lld-${LLVM_VERSION}

# Create symlinks
sudo ln -sf /usr/bin/clang-${LLVM_VERSION} /usr/bin/clang
sudo ln -sf /usr/bin/clang-tidy-${LLVM_VERSION} /usr/bin/clang-tidy
sudo ln -sf /usr/bin/clang-format-${LLVM_VERSION} /usr/bin/clang-format
sudo ln -sf /usr/bin/lld-${LLVM_VERSION} /usr/bin/lld
sudo ln -sf /usr/bin/ld.lld-${LLVM_VERSION} /usr/bin/ld.lld
```

### RISC-V Cross-Compilation (Ubuntu/Debian)

For bare-metal RISC-V builds and running tests via QEMU:

```bash
sudo apt install gcc-riscv64-unknown-elf qemu-user
```

## Quick Start

```bash
# Clone the repository
git clone https://github.com/daniellehrner/div0.git
cd div0

# Build (debug)
make debug

# Run tests
make test

# Build release
make release
```

## Build Targets

| Target             | Description                              |
|--------------------|------------------------------------------|
| `make debug`       | Debug build with ASan + UBSan            |
| `make release`     | Optimized release build with LTO         |
| `make threadsan`   | Build with ThreadSanitizer               |
| `make bare-metal-riscv` | RISC-V 64-bit bare-metal build      |
| `make test`        | Run unit tests                           |
| `make format`      | Format code with clang-format            |
| `make clang-tidy`  | Run static analysis                      |

## Cross-Compilation

### RISC-V Bare-Metal

For zkVM or other bare-metal RISC-V targets. Dependencies (picolibc, compiler-rt) are built automatically from source:

```bash
# Build and test via QEMU
cmake --preset riscv64
cmake --build build/riscv64
ctest --test-dir build/riscv64
```

Requirements:
- QEMU RISC-V 64-bit user-mode emulator (`qemu-riscv64`) for running tests