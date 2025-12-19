# div0 - High-performance EVM implementation
# Makefile wrapper for CMake builds

.PHONY: all check debug release threadsan bare-metal-riscv test clean format clang-tidy help

# Default compiler - must be Clang
CC := clang

# Build directories
BUILD_DEBUG = build/debug
BUILD_RELEASE = build/release
BUILD_THREADSAN = build/threadsan
BUILD_RISCV = build/riscv64

# Default target
all: debug

# Debug build with ASan + UBSan
debug:
	cmake -B $(BUILD_DEBUG) -G Ninja \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DEBUG)

# Release build with LTO
release:
	cmake -B $(BUILD_RELEASE) -G Ninja \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_RELEASE)

# ThreadSanitizer build
threadsan:
	cmake -B $(BUILD_THREADSAN) -G Ninja \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_BUILD_TYPE=ThreadSan
	cmake --build $(BUILD_THREADSAN)

# RISC-V bare-metal build with picolibc
# Requires PICOLIBC_ROOT to be set
bare-metal-riscv:
ifndef PICOLIBC_ROOT
	$(error PICOLIBC_ROOT is not set. Set it to your picolibc installation path.)
endif
	cmake -B $(BUILD_RISCV) -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-picolibc.cmake \
		-DPICOLIBC_ROOT=$(PICOLIBC_ROOT) \
		-DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_RISCV)

# Run tests (debug build)
test: debug
	ctest --test-dir $(BUILD_DEBUG) --output-on-failure

# Run tests with ThreadSanitizer
test-threadsan: threadsan
	ctest --test-dir $(BUILD_THREADSAN) --output-on-failure

# Clean all build artifacts
clean:
	rm -rf build

# Format source code
format:
	find src include tests -name '*.c' -o -name '*.h' | xargs clang-format -i

# Check formatting without modifying files
format-check:
	find src include tests -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror

# Run clang-tidy
clang-tidy: debug
	find src include -name '*.c' -o -name '*.h' | xargs clang-tidy -p $(BUILD_DEBUG)

check: format-check clang-tidy

# Help
help:
	@echo "div0 build targets:"
	@echo "  debug            - Debug build with ASan + UBSan (default)"
	@echo "  release          - Release build with LTO"
	@echo "  threadsan        - Build with ThreadSanitizer"
	@echo "  bare-metal-riscv - RISC-V 64-bit bare-metal build (requires PICOLIBC_ROOT)"
	@echo "  test             - Run tests (debug build)"
	@echo "  test-threadsan   - Run tests with ThreadSanitizer"
	@echo "  clean            - Remove all build artifacts"
	@echo "  format           - Format source code with clang-format"
	@echo "  format-check     - Check formatting without modifying"
	@echo "  clang-tidy       - Run clang-tidy static analysis"
	@echo "  help             - Show this help message"