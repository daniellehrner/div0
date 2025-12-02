// Benchmarks for Uint256 comparison operations: ==, !=, <, >, <=, >=

#include <benchmark/benchmark.h>

#include <random>

#include "div0/types/uint256.h"

using namespace div0::types;

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp, *-avoid-non-const-global-variables)

static auto random_uint256() -> Uint256 { return Uint256(rng(), rng(), rng(), rng()); }

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// ============================================================================
// Equality Benchmarks
// ============================================================================

static void bm_uint256_equal(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a == b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_equal);

// Equal to self (always true, tests early exit optimization)
static void bm_uint256_equal_self(benchmark::State& state) {
  const auto a = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    // NOLINTNEXTLINE(misc-redundant-expression) - intentionally comparing to self
    auto result = (a == a);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_equal_self);

static void bm_uint256_not_equal(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a != b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_not_equal);

// ============================================================================
// Less Than Benchmarks
// ============================================================================

static void bm_uint256_less_than(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a < b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_less_than);

// Compare with zero (common check)
static void bm_uint256_less_than_zero(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto ZERO = Uint256(0);

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a < ZERO);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_less_than_zero);

// Compare adjacent values (differ only in low limb)
static void bm_uint256_less_than_adjacent(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = a + Uint256(1);

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a < b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_less_than_adjacent);

static void bm_uint256_less_or_equal(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a <= b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_less_or_equal);

// ============================================================================
// Greater Than Benchmarks
// ============================================================================

static void bm_uint256_greater_than(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a > b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_greater_than);

// Compare with zero (common check)
static void bm_uint256_greater_than_zero(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto ZERO = Uint256(0);

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a > ZERO);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_greater_than_zero);

static void bm_uint256_greater_or_equal(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a >= b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_greater_or_equal);

// ============================================================================
// EVM-Specific Comparison Patterns
// ============================================================================

// ISZERO opcode pattern
static void bm_uint256_is_zero_check(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto ZERO = Uint256(0);

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a == ZERO);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_is_zero_check);

// Compare max value
static void bm_uint256_compare_max(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto MAX = Uint256::max();

  for ([[maybe_unused]] auto _ : state) {
    auto result = (a < MAX);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_compare_max);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
