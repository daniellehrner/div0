.PHONY: debug release test bench fuzz format clean clang-tidy

# Platform detection
UNAME_S := $(shell uname -s)

# Configure and build debug (with sanitizers)
debug:
	cmake --preset=debug
	cmake --build build/debug

# Configure and build release
release:
	cmake --preset=release
	cmake --build build/release

# Build and run tests
# Usage: make test                      - run all tests
#        make test ARGS="-L unit"       - run only unit tests
#        make test ARGS="-LE integration" - exclude integration tests
#        make test ARGS="-R Swap"       - run tests matching "Swap"
test:
	cmake --preset=debug
	cmake --build build/debug --target div0_unit_tests div0_integration_tests
	ctest --preset=debug --output-on-failure $(ARGS)

# Build and run benchmarks
# Usage: make bench ARGS="--benchmark_filter=Div"
bench:
	cmake --preset=release
	cmake --build build/release --target div0_bench
	./build/release/benchmarks/div0_bench $(ARGS)

# Build fuzzers (macOS requires Homebrew LLVM for libFuzzer)
# Usage: make fuzz TARGET=evm_fuzzer
ifeq ($(UNAME_S),Darwin)
  FUZZ_CC := /opt/homebrew/opt/llvm/bin/clang
  FUZZ_CXX := /opt/homebrew/opt/llvm/bin/clang++
else
  FUZZ_CC := clang
  FUZZ_CXX := clang++
endif

fuzz:
	CC=$(FUZZ_CC) CXX=$(FUZZ_CXX) cmake --preset=fuzz
	cmake --build build/fuzz
	@echo ""
	@echo "Run a fuzzer: ./build/fuzz/fuzz/<fuzzer_name> [corpus_dir]"
	@echo "Example: ./build/fuzz/fuzz/evm_fuzzer corpus/evm/"

# Run clang-tidy on all source files (requires compile_commands.json)
# macOS needs -isysroot to find system headers like <iostream>
ifeq ($(UNAME_S),Darwin)
  CLANG_TIDY_EXTRA_ARGS := -extra-arg=-isysroot -extra-arg=$(shell xcrun --show-sdk-path)
  NPROC := $(shell sysctl -n hw.ncpu)
else
  CLANG_TIDY_EXTRA_ARGS :=
  NPROC := $(shell nproc)
endif

# Run clang-tidy on ALL source files (main sources, tests, benchmarks, fuzzers)
# This requires building multiple presets to get all compile_commands.json entries
clang-tidy:
	cmake --preset=debug
	cmake --preset=release
	CC=$(FUZZ_CC) CXX=$(FUZZ_CXX) cmake --preset=fuzz
	cmake --build build/debug --target xkcp_build
	cmake --build build/release --target xkcp_build
	cmake --build build/fuzz --target xkcp_build
	run-clang-tidy -p build/debug -j $(NPROC) -warnings-as-errors='*' -source-filter='^((?!_deps).)*$$' $(CLANG_TIDY_EXTRA_ARGS)
	run-clang-tidy -p build/release -j $(NPROC) -warnings-as-errors='*' -source-filter='^.*/benchmarks/.*$$' $(CLANG_TIDY_EXTRA_ARGS)
	run-clang-tidy -p build/fuzz -j $(NPROC) -warnings-as-errors='*' -source-filter='^.*/fuzz/[^_].*$$' $(CLANG_TIDY_EXTRA_ARGS)

# Format all files according to the .clang-format rules
format:
	find benchmarks fuzz include src tests -name '*.cpp' -o -name '*.h' | xargs clang-format -i

# Check if all files are formatted correctly
format-check:
	find benchmarks fuzz include src tests -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run -Werror

check: format-check clang-tidy
	@echo "All static analysis checks passed!"

# Clean build directories
clean:
	rm -rf build/debug build/release
