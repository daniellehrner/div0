// Benchmarks for Uint256 bitwise operations: AND, OR, XOR, NOT

#include <benchmark/benchmark.h>

#include <random>

#include "div0/types/uint256.h"

using namespace div0::types;

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp, *-avoid-non-const-global-variables)

static auto random_uint256() -> Uint256 { return Uint256(rng(), rng(), rng(), rng()); }

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// ============================================================================
// AND Benchmarks
// ============================================================================

static void bm_uint256_and(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = a & b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_and);

// AND with mask (common for extracting bits)
static void bm_uint256_and_mask(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto MASK = Uint256(UINT64_MAX);  // Low 64 bits mask

  for ([[maybe_unused]] auto _ : state) {
    auto result = a & MASK;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_and_mask);

// AND with zero
static void bm_uint256_and_zero(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto ZERO = Uint256(0);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a & ZERO;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_and_zero);

// ============================================================================
// OR Benchmarks
// ============================================================================

static void bm_uint256_or(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = a | b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_or);

// OR with zero (identity operation)
static void bm_uint256_or_zero(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto ZERO = Uint256(0);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a | ZERO;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_or_zero);

// ============================================================================
// XOR Benchmarks
// ============================================================================

static void bm_uint256_xor(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = a ^ b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_xor);

// XOR with self (should be zero)
static void bm_uint256_xor_self(benchmark::State& state) {
  const auto a = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    // NOLINTNEXTLINE(misc-redundant-expression) - intentionally for benchmarking
    auto result = a ^ a;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_xor_self);

// ============================================================================
// NOT Benchmarks
// ============================================================================

static void bm_uint256_not(benchmark::State& state) {
  const auto a = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = ~a;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_not);

// Double NOT (should return original)
static void bm_uint256_not_not(benchmark::State& state) {
  const auto a = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = ~~a;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_not_not);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
