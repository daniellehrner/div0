// Benchmarks for memory opcodes: MLOAD, MSTORE, MSTORE8, MSIZE

#include <benchmark/benchmark.h>

#include <random>

#include "../src/evm/opcodes/memory.h"
#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/types/uint256.h"

using namespace div0::evm;
using namespace div0::evm::opcodes;
using namespace div0::types;

// Gas costs from EVM specification
constexpr uint64_t GAS_MLOAD = 3;
constexpr uint64_t GAS_MSTORE = 3;
constexpr uint64_t GAS_MSTORE8 = 3;
constexpr uint64_t GAS_MSIZE = 2;

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp)

// Helper to generate random Uint256
static Uint256 random_uint256() { return Uint256(rng(), rng(), rng(), rng()); }

// ============================================================================
// MSTORE Opcode Benchmarks
// ============================================================================

static void bm_opcode_mstore(benchmark::State& state) {
  Stack stack;
  Memory memory;
  memory.expand(1024);

  const auto value = random_uint256();
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(Uint256(0));
    (void)stack.push(value);

    auto result = mstore(stack, gas, GAS_MSTORE, gas::memory::access_cost, memory);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_mstore);

// ============================================================================
// MLOAD Opcode Benchmarks
// ============================================================================

static void bm_opcode_mload(benchmark::State& state) {
  Stack stack;
  Memory memory;
  memory.expand(1024);
  memory.store_word_unsafe(0, random_uint256());

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(Uint256(0));

    auto result = mload(stack, gas, GAS_MLOAD, gas::memory::access_cost, memory);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_mload);

// ============================================================================
// MSTORE8 Opcode Benchmarks
// ============================================================================

static void bm_opcode_mstore8(benchmark::State& state) {
  Stack stack;
  Memory memory;
  memory.expand(1024);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(Uint256(0));
    (void)stack.push(Uint256(0x42));

    auto result = mstore8(stack, gas, GAS_MSTORE8, gas::memory::access_cost, memory);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_mstore8);

// ============================================================================
// MSIZE Opcode Benchmarks
// ============================================================================

static void bm_opcode_msize(benchmark::State& state) {
  Stack stack;
  Memory memory;
  memory.expand(1024);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = msize(stack, gas, GAS_MSIZE, memory);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_msize);

// ============================================================================
// Memory Expansion Benchmarks
// ============================================================================
// With 60M gas, max memory is ~174,500 words (~5.6MB)
// Formula: cost = 3 * words + words² / 512

static void bm_memory_expand(benchmark::State& state) {
  const auto bytes = static_cast<uint64_t>(state.range(0));

  for ([[maybe_unused]] auto _ : state) {
    Memory memory;
    memory.expand(bytes);
    benchmark::DoNotOptimize(memory);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(bytes));
}
BENCHMARK(bm_memory_expand)
    ->Arg(32)        // 1 word
    ->Arg(1024)      // 32 words (1KB)
    ->Arg(32768)     // 1024 words (32KB)
    ->Arg(131072)    // 4096 words (128KB)
    ->Arg(1048576)   // 32768 words (1MB)
    ->Arg(5584128);  // ~174504 words (~5.6MB, max at 60M gas)

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
