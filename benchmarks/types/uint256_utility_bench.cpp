// Benchmarks for Uint256 utility methods: is_zero(), count_leading_zeros(), constructors

#include <benchmark/benchmark.h>

#include <random>

#include "div0/types/uint256.h"

using namespace div0::types;

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp, *-avoid-non-const-global-variables)

static auto random_uint256() -> Uint256 { return Uint256(rng(), rng(), rng(), rng()); }

static auto random_uint64() -> uint64_t { return rng(); }

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// ============================================================================
// is_zero() Benchmarks
// ============================================================================

static void bm_uint256_is_zero_random(benchmark::State& state) {
  const auto a = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = a.is_zero();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_is_zero_random);

static void bm_uint256_is_zero_actual_zero(benchmark::State& state) {
  constexpr auto ZERO = Uint256(0);

  for ([[maybe_unused]] auto _ : state) {
    auto result = ZERO.is_zero();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_is_zero_actual_zero);

// Non-zero only in high limb (worst case for early-exit optimization)
static void bm_uint256_is_zero_high_limb(benchmark::State& state) {
  constexpr auto VALUE = Uint256(0, 0, 0, 1);

  for ([[maybe_unused]] auto _ : state) {
    auto result = VALUE.is_zero();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_is_zero_high_limb);

// ============================================================================
// count_leading_zeros() Benchmarks
// ============================================================================

static void bm_uint256_clz_random(benchmark::State& state) {
  const auto a = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    auto result = a.count_leading_zeros();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_clz_random);

static void bm_uint256_clz_zero(benchmark::State& state) {
  constexpr auto ZERO = Uint256(0);

  for ([[maybe_unused]] auto _ : state) {
    auto result = ZERO.count_leading_zeros();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_clz_zero);

static void bm_uint256_clz_max(benchmark::State& state) {
  constexpr auto MAX = Uint256::max();

  for ([[maybe_unused]] auto _ : state) {
    auto result = MAX.count_leading_zeros();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_clz_max);

// Value in low limb only (CLZ must check all higher limbs)
static void bm_uint256_clz_low_limb(benchmark::State& state) {
  const auto value = Uint256(random_uint64());

  for ([[maybe_unused]] auto _ : state) {
    auto result = value.count_leading_zeros();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_clz_low_limb);

// Value in high limb (CLZ finds it immediately)
static void bm_uint256_clz_high_limb(benchmark::State& state) {
  const auto value = Uint256(0, 0, 0, random_uint64());

  for ([[maybe_unused]] auto _ : state) {
    auto result = value.count_leading_zeros();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_clz_high_limb);

// ============================================================================
// Constructor Benchmarks
// ============================================================================

static void bm_uint256_construct_default(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    auto result = Uint256();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_construct_default);

static void bm_uint256_construct_single(benchmark::State& state) {
  const auto value = random_uint64();

  for ([[maybe_unused]] auto _ : state) {
    auto result = Uint256(value);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_construct_single);

static void bm_uint256_construct_four_limbs(benchmark::State& state) {
  const auto l0 = random_uint64();
  const auto l1 = random_uint64();
  const auto l2 = random_uint64();
  const auto l3 = random_uint64();

  for ([[maybe_unused]] auto _ : state) {
    auto result = Uint256(l0, l1, l2, l3);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_construct_four_limbs);

// ============================================================================
// Factory Method Benchmarks
// ============================================================================

static void bm_uint256_factory_zero(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    auto result = Uint256::zero();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_factory_zero);

static void bm_uint256_factory_one(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    auto result = Uint256::one();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_factory_one);

static void bm_uint256_factory_max(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    auto result = Uint256::max();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_factory_max);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
