// Benchmarks for stack opcodes: PUSH0, PUSH1-PUSH32, DUP1-DUP16, SWAP1-SWAP16

#include <benchmark/benchmark.h>

#include <random>
#include <vector>

#include "../src/evm/opcodes/dup.h"
#include "../src/evm/opcodes/push.h"
#include "../src/evm/opcodes/swap.h"
#include "div0/evm/stack.h"
#include "div0/types/uint256.h"

using namespace div0::evm;
using namespace div0::evm::opcodes;
using namespace div0::types;

// Gas costs from EVM specification
constexpr uint64_t GAS_PUSH0 = 2;
constexpr uint64_t GAS_PUSH = 3;
constexpr uint64_t GAS_DUP = 3;
constexpr uint64_t GAS_SWAP = 3;

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp)

// Helper to generate random bytecode of given size
static std::vector<uint8_t> random_bytecode(size_t size) {
  std::vector<uint8_t> code(size);
  for (auto& b : code) {
    b = static_cast<uint8_t>(rng() & 0xFF);
  }
  return code;
}

// ============================================================================
// PUSH0 Opcode Benchmarks
// ============================================================================

static void bm_opcode_push0(benchmark::State& state) {
  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = push0(stack, gas, GAS_PUSH0);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_push0);

// ============================================================================
// PUSH1 Opcode Benchmarks
// ============================================================================

static void bm_opcode_push1(benchmark::State& state) {
  const auto code = random_bytecode(1);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    uint64_t pc = 0;
    auto result = push_n<1>(stack, gas, GAS_PUSH, code, pc);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_push1);

// ============================================================================
// PUSH2 Opcode Benchmarks
// ============================================================================

static void bm_opcode_push2(benchmark::State& state) {
  const auto code = random_bytecode(2);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    uint64_t pc = 0;
    auto result = push_n<2>(stack, gas, GAS_PUSH, code, pc);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_push2);

// ============================================================================
// PUSH4 Opcode Benchmarks
// ============================================================================

static void bm_opcode_push4(benchmark::State& state) {
  const auto code = random_bytecode(4);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    uint64_t pc = 0;
    auto result = push_n<4>(stack, gas, GAS_PUSH, code, pc);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_push4);

// ============================================================================
// PUSH8 Opcode Benchmarks
// ============================================================================

static void bm_opcode_push8(benchmark::State& state) {
  const auto code = random_bytecode(8);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    uint64_t pc = 0;
    auto result = push_n<8>(stack, gas, GAS_PUSH, code, pc);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_push8);

// ============================================================================
// PUSH16 Opcode Benchmarks
// ============================================================================

static void bm_opcode_push16(benchmark::State& state) {
  const auto code = random_bytecode(16);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    uint64_t pc = 0;
    auto result = push_n<16>(stack, gas, GAS_PUSH, code, pc);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_push16);

// ============================================================================
// PUSH20 Opcode Benchmarks (Ethereum Address Size)
// ============================================================================

static void bm_opcode_push20(benchmark::State& state) {
  const auto code = random_bytecode(20);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    uint64_t pc = 0;
    auto result = push_n<20>(stack, gas, GAS_PUSH, code, pc);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_push20);

// ============================================================================
// PUSH32 Opcode Benchmarks (Full 256-bit Value)
// ============================================================================

static void bm_opcode_push32(benchmark::State& state) {
  const auto code = random_bytecode(32);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    uint64_t pc = 0;
    auto result = push_n<32>(stack, gas, GAS_PUSH, code, pc);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_push32);

// ============================================================================
// PUSH32 with Truncated Bytecode (Edge Case)
// ============================================================================

static void bm_opcode_push32_truncated(benchmark::State& state) {
  // Only 16 bytes available for PUSH32
  const auto code = random_bytecode(16);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    uint64_t pc = 0;
    auto result = push_n<32>(stack, gas, GAS_PUSH, code, pc);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_push32_truncated);

// ============================================================================
// Multiple Sequential PUSHes (Simulating Real Bytecode)
// ============================================================================

static void bm_opcode_push_sequence(benchmark::State& state) {
  // Simulate: PUSH1 PUSH1 PUSH1 (3 pushes)
  const auto code = random_bytecode(1);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    uint64_t pc = 0;
    push_n<1>(stack, gas, GAS_PUSH, code, pc);
    pc = 0;
    push_n<1>(stack, gas, GAS_PUSH, code, pc);
    pc = 0;
    push_n<1>(stack, gas, GAS_PUSH, code, pc);

    benchmark::DoNotOptimize(stack);

    stack.pop_unsafe();
    stack.pop_unsafe();
    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations() * 3);
}
BENCHMARK(bm_opcode_push_sequence);

// Helper to generate random Uint256
static Uint256 random_uint256() { return Uint256(rng(), rng(), rng(), rng()); }

// Helper to fill stack with n random elements
static void fill_stack(Stack& stack, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    (void)stack.push(random_uint256());
  }
}

// ============================================================================
// DUP1 Opcode Benchmarks
// ============================================================================

static void bm_opcode_dup1(benchmark::State& state) {
  Stack stack;
  fill_stack(stack, 1);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = dup_n(stack, gas, GAS_DUP, 1);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_dup1);

// ============================================================================
// DUP2 Opcode Benchmarks
// ============================================================================

static void bm_opcode_dup2(benchmark::State& state) {
  Stack stack;
  fill_stack(stack, 2);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = dup_n(stack, gas, GAS_DUP, 2);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_dup2);

// ============================================================================
// DUP8 Opcode Benchmarks
// ============================================================================

static void bm_opcode_dup8(benchmark::State& state) {
  Stack stack;
  fill_stack(stack, 8);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = dup_n(stack, gas, GAS_DUP, 8);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_dup8);

// ============================================================================
// DUP16 Opcode Benchmarks (Maximum Depth)
// ============================================================================

static void bm_opcode_dup16(benchmark::State& state) {
  Stack stack;
  fill_stack(stack, 16);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = dup_n(stack, gas, GAS_DUP, 16);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_dup16);

// ============================================================================
// DUP Sequence Benchmarks
// ============================================================================

static void bm_opcode_dup_sequence(benchmark::State& state) {
  // Simulate: DUP1 DUP2 DUP1 (common pattern)
  Stack stack;
  fill_stack(stack, 2);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    dup_n(stack, gas, GAS_DUP, 1);
    dup_n(stack, gas, GAS_DUP, 2);
    dup_n(stack, gas, GAS_DUP, 1);

    benchmark::DoNotOptimize(stack);

    stack.pop_unsafe();
    stack.pop_unsafe();
    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations() * 3);
}
BENCHMARK(bm_opcode_dup_sequence);

// ============================================================================
// SWAP1 Opcode Benchmarks
// ============================================================================

static void bm_opcode_swap1(benchmark::State& state) {
  Stack stack;
  fill_stack(stack, 2);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = swap_n(stack, gas, GAS_SWAP, 1);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_swap1);

// ============================================================================
// SWAP2 Opcode Benchmarks
// ============================================================================

static void bm_opcode_swap2(benchmark::State& state) {
  Stack stack;
  fill_stack(stack, 3);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = swap_n(stack, gas, GAS_SWAP, 2);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_swap2);

// ============================================================================
// SWAP8 Opcode Benchmarks
// ============================================================================

static void bm_opcode_swap8(benchmark::State& state) {
  Stack stack;
  fill_stack(stack, 9);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = swap_n(stack, gas, GAS_SWAP, 8);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_swap8);

// ============================================================================
// SWAP16 Opcode Benchmarks (Maximum Depth)
// ============================================================================

static void bm_opcode_swap16(benchmark::State& state) {
  Stack stack;
  fill_stack(stack, 17);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    auto result = swap_n(stack, gas, GAS_SWAP, 16);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_swap16);

// ============================================================================
// SWAP Sequence Benchmarks
// ============================================================================

static void bm_opcode_swap_sequence(benchmark::State& state) {
  // Simulate: SWAP1 SWAP2 SWAP1 (common pattern for reordering)
  Stack stack;
  fill_stack(stack, 4);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    swap_n(stack, gas, GAS_SWAP, 1);
    swap_n(stack, gas, GAS_SWAP, 2);
    swap_n(stack, gas, GAS_SWAP, 1);

    benchmark::DoNotOptimize(stack);
  }

  state.SetItemsProcessed(state.iterations() * 3);
}
BENCHMARK(bm_opcode_swap_sequence);

// ============================================================================
// Double SWAP (Restoring Original - Common Pattern)
// ============================================================================

static void bm_opcode_swap_double(benchmark::State& state) {
  // SWAP1 twice restores original order
  Stack stack;
  fill_stack(stack, 2);

  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    swap_n(stack, gas, GAS_SWAP, 1);
    swap_n(stack, gas, GAS_SWAP, 1);

    benchmark::DoNotOptimize(stack);
  }

  state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(bm_opcode_swap_double);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
