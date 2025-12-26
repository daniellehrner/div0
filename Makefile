# div0 - High-performance EVM implementation
# Makefile wrapper for CMake builds

.PHONY: all check debug release threadsan bare-metal-riscv test clean distclean format clang-tidy semgrep help

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

# Clean project build artifacts (preserves external dependencies like picolibc)
clean:
	@for dir in $(BUILD_DEBUG) $(BUILD_RELEASE) $(BUILD_THREADSAN) $(BUILD_RISCV); do \
		if [ -d "$$dir" ]; then \
			echo "Cleaning $$dir (preserving external deps)..."; \
			rm -f "$$dir"/*.a "$$dir"/div0_tests "$$dir"/div0_tests.exe; \
			rm -rf "$$dir"/CMakeFiles; \
			rm -f "$$dir"/CMakeCache.txt "$$dir"/cmake_install.cmake; \
			rm -f "$$dir"/build.ninja "$$dir"/.ninja_* "$$dir"/compile_commands.json; \
			rm -f "$$dir"/CTestTestfile.cmake; \
		fi; \
	done

# Deep clean - removes everything including external dependencies
distclean:
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

# Run semgrep C23 checks (only fail on ERROR, not WARNING)
semgrep:
	semgrep --config .semgrep/c23.yaml --error --severity=ERROR

check: format-check clang-tidy semgrep

# Help
help:
	@echo "div0 build targets:"
	@echo "  debug            - Debug build with ASan + UBSan (default)"
	@echo "  release          - Release build with LTO"
	@echo "  threadsan        - Build with ThreadSanitizer"
	@echo "  bare-metal-riscv - RISC-V 64-bit bare-metal build (requires PICOLIBC_ROOT)"
	@echo "  test             - Run tests (debug build)"
	@echo "  test-threadsan   - Run tests with ThreadSanitizer"
	@echo "  clean            - Clean project artifacts (preserves external deps)"
	@echo "  distclean        - Remove everything including external dependencies"
	@echo "  format           - Format source code with clang-format"
	@echo "  format-check     - Check formatting without modifying"
	@echo "  clang-tidy       - Run clang-tidy static analysis"
	@echo "  semgrep          - Run semgrep C23 checks"
	@echo "  check            - Run all static analysis (format-check, clang-tidy, semgrep)"
	@echo "  help             - Show this help message"