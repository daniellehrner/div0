// Benchmarks for comparison opcodes: LT, GT, EQ, ISZERO

#include <benchmark/benchmark.h>

#include <random>

#include "div0/evm/stack.h"
#include "div0/types/uint256.h"

// clang-format off
#include "../src/evm/opcodes/op_helpers.h"
#include "../src/evm/opcodes/comparison.h"
// clang-format on

using namespace div0::evm;
using namespace div0::evm::opcodes;
using namespace div0::types;

// Gas costs from EVM specification
constexpr uint64_t GAS_LT = 3;
constexpr uint64_t GAS_GT = 3;
constexpr uint64_t GAS_EQ = 3;
constexpr uint64_t GAS_ISZERO = 3;

// Maximum uint64 for multi-limb tests
constexpr uint64_t MAX64 = UINT64_MAX;

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp)

static Uint256 random_uint256() { return Uint256(rng(), rng(), rng(), rng()); }

// ============================================================================
// LT (Less Than) Opcode Benchmarks
// ============================================================================

static void bm_opcode_lt(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = lt(stack, gas, GAS_LT);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_lt);

// LT with values that differ in high limb (early exit)
static void bm_opcode_lt_high_limb_diff(benchmark::State& state) {
  constexpr auto A = Uint256(0, 0, 0, 1);
  constexpr auto B = Uint256(0, 0, 0, 2);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(B);
    (void)stack.push(A);

    auto result = lt(stack, gas, GAS_LT);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_lt_high_limb_diff);

// LT with values that differ in low limb (full comparison)
static void bm_opcode_lt_low_limb_diff(benchmark::State& state) {
  constexpr auto A = Uint256(1, MAX64, MAX64, MAX64);
  constexpr auto B = Uint256(2, MAX64, MAX64, MAX64);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(B);
    (void)stack.push(A);

    auto result = lt(stack, gas, GAS_LT);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_lt_low_limb_diff);

// ============================================================================
// GT (Greater Than) Opcode Benchmarks
// ============================================================================

static void bm_opcode_gt(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = gt(stack, gas, GAS_GT);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_gt);

// GT with values that differ in high limb (early exit)
static void bm_opcode_gt_high_limb_diff(benchmark::State& state) {
  constexpr auto A = Uint256(0, 0, 0, 2);
  constexpr auto B = Uint256(0, 0, 0, 1);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(B);
    (void)stack.push(A);

    auto result = gt(stack, gas, GAS_GT);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_gt_high_limb_diff);

// ============================================================================
// EQ (Equality) Opcode Benchmarks
// ============================================================================

static void bm_opcode_eq(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(b);
    (void)stack.push(a);

    auto result = eq(stack, gas, GAS_EQ);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_eq);

// EQ with equal values (full comparison)
static void bm_opcode_eq_equal(benchmark::State& state) {
  const Uint256 a = random_uint256();

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(a);
    (void)stack.push(a);

    auto result = eq(stack, gas, GAS_EQ);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_eq_equal);

// EQ with values that differ in high limb (early exit)
static void bm_opcode_eq_high_limb_diff(benchmark::State& state) {
  constexpr auto A = Uint256(MAX64, MAX64, MAX64, 1);
  constexpr auto B = Uint256(MAX64, MAX64, MAX64, 2);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(B);
    (void)stack.push(A);

    auto result = eq(stack, gas, GAS_EQ);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_eq_high_limb_diff);

// EQ with values that differ only in low limb (full comparison)
static void bm_opcode_eq_low_limb_diff(benchmark::State& state) {
  constexpr auto A = Uint256(1, MAX64, MAX64, MAX64);
  constexpr auto B = Uint256(2, MAX64, MAX64, MAX64);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(B);
    (void)stack.push(A);

    auto result = eq(stack, gas, GAS_EQ);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_eq_low_limb_diff);

// ============================================================================
// ISZERO Opcode Benchmarks
// ============================================================================

static void bm_opcode_iszero(benchmark::State& state) {
  const Uint256 a = random_uint256();

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(a);

    auto result = is_zero(stack, gas, GAS_ISZERO);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_iszero);

// ISZERO with zero value
static void bm_opcode_iszero_zero(benchmark::State& state) {
  constexpr auto ZERO = Uint256(0);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(ZERO);

    auto result = is_zero(stack, gas, GAS_ISZERO);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_iszero_zero);

// ISZERO with value that has only high limb set (tests all limb checks)
static void bm_opcode_iszero_high_limb_only(benchmark::State& state) {
  constexpr auto A = Uint256(0, 0, 0, 1);

  Stack stack;
  auto gas = UINT64_MAX;

  for ([[maybe_unused]] auto _ : state) {
    (void)stack.push(A);

    auto result = is_zero(stack, gas, GAS_ISZERO);
    benchmark::DoNotOptimize(result);

    stack.pop_unsafe();
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_opcode_iszero_high_limb_only);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
