// Benchmarks for arithmetic opcodes: ADD, SUB, MUL, DIV, MOD

#include <benchmark/benchmark.h>

#include <random>

#include "../src/evm/opcodes/arithmetic.h"
#include "div0/evm/stack.h"
#include "div0/types/uint256.h"

using namespace div0::evm;
using namespace div0::evm::opcodes;
using namespace div0::types;

// Gas costs from EVM specification
constexpr uint64_t GAS_ADD = 3;
constexpr uint64_t GAS_SUB = 3;
constexpr uint64_t GAS_MUL = 5;
constexpr uint64_t GAS_DIV = 5;
constexpr uint64_t GAS_MOD = 5;

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp)

static Uint256 random_uint256() { return Uint256(rng(), rng(), rng(), rng()); }

static uint64_t random_uint64() { return rng(); }

// Helper to ensure dividend > divisor for multi-limb division
static Uint256 smaller_than(const Uint256& dividend) {
  const unsigned shift = (rng() % 128) + 1;
  return dividend >> Uint256(shift);
}

// ============================================================================
// ADD Opcode Benchmarks
// ============================================================================

static void bm_opcode_add(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = add(stack, gas, GAS_ADD);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_add);

// ADD with small values (common case - single limb)
static void bm_opcode_add_small(benchmark::State& state) {
  const Uint256 a(random_uint64());
  const Uint256 b(random_uint64());

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = add(stack, gas, GAS_ADD);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_add_small);

// ADD with max carry propagation
static void bm_opcode_add_max_carry(benchmark::State& state) {
  const auto max_val = ~Uint256(0);
  constexpr auto ONE = Uint256(1);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(ONE);
    (void)stack.push(max_val);

    auto result = add(stack, gas, GAS_ADD);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_add_max_carry);

// ============================================================================
// SUB Opcode Benchmarks
// ============================================================================

static void bm_opcode_sub(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = sub(stack, gas, GAS_SUB);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_sub);

// SUB with max borrow propagation (0 - 1 = MAX)
static void bm_opcode_sub_max_borrow(benchmark::State& state) {
  constexpr auto ZERO = Uint256(0);
  constexpr auto ONE = Uint256(1);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(ONE);
    (void)stack.push(ZERO);

    auto result = sub(stack, gas, GAS_SUB);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_sub_max_borrow);

// ============================================================================
// MUL Opcode Benchmarks
// ============================================================================

static void bm_opcode_mul(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = mul(stack, gas, GAS_MUL);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_mul);

// MUL with small multiplier (common for gas calculations)
static void bm_opcode_mul_small(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b(random_uint64());

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = mul(stack, gas, GAS_MUL);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_mul_small);

// ============================================================================
// DIV Opcode Benchmarks
// ============================================================================

// Multi-limb divisor (Knuth Algorithm D path)
static void bm_opcode_div(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = smaller_than(a);  // Guarantees multi-limb division

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = div(stack, gas, GAS_DIV);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_div);

// DIV with small divisor (fast path - single limb division)
static void bm_opcode_div_small(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b(random_uint64());

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = div(stack, gas, GAS_DIV);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_div_small);

// DIV by zero (EVM returns 0)
static void bm_opcode_div_by_zero(benchmark::State& state) {
  const Uint256 a = random_uint256();
  constexpr auto ZERO = Uint256(0);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(ZERO);
    (void)stack.push(a);

    auto result = div(stack, gas, GAS_DIV);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_div_by_zero);

// ============================================================================
// MOD Opcode Benchmarks
// ============================================================================

// Multi-limb divisor
static void bm_opcode_mod(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = smaller_than(a);  // Guarantees multi-limb division

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = mod(stack, gas, GAS_MOD);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_mod);

// MOD with small divisor
static void bm_opcode_mod_small(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b(random_uint64());

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = mod(stack, gas, GAS_MOD);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_mod_small);

// MOD by zero (EVM returns 0)
static void bm_opcode_mod_by_zero(benchmark::State& state) {
  const Uint256 a = random_uint256();
  constexpr auto ZERO = Uint256(0);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(ZERO);
    (void)stack.push(a);

    auto result = mod(stack, gas, GAS_MOD);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_mod_by_zero);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
