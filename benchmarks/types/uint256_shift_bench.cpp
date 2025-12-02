// Benchmarks for Uint256 SHIFT operations: SHL (<<), SHR (>>)

#include <benchmark/benchmark.h>

#include <random>

#include "div0/types/uint256.h"

using namespace div0::types;

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp, *-avoid-non-const-global-variables)

static auto random_uint256() -> Uint256 { return Uint256(rng(), rng(), rng(), rng()); }

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// ============================================================================
// Left Shift Benchmarks
// ============================================================================

// Small SHIFT (within first limb)
static void bm_uint256_shl_small(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(7);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a << SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shl_small);

// Shift by exactly one limb (64 bits)
static void bm_uint256_shl_one_limb(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(64);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a << SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shl_one_limb);

// Cross-limb SHIFT (requires bit manipulation across limbs)
static void bm_uint256_shl_cross_limb(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(100);  // Crosses limb boundary

  for ([[maybe_unused]] auto _ : state) {
    auto result = a << SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shl_cross_limb);

// Large SHIFT (most bits shifted out)
static void bm_uint256_shl_large(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(200);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a << SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shl_large);

// Shift by 256+ (EVM semantics: returns 0)
static void bm_uint256_shl_overflow(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(300);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a << SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shl_overflow);

// Shift by 1 (common operation)
static void bm_uint256_shl_one(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(1);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a << SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shl_one);

// ============================================================================
// Right Shift Benchmarks
// ============================================================================

// Small SHIFT (within first limb)
static void bm_uint256_shr_small(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(7);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a >> SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shr_small);

// Shift by exactly one limb (64 bits)
static void bm_uint256_shr_one_limb(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(64);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a >> SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shr_one_limb);

// Cross-limb SHIFT
static void bm_uint256_shr_cross_limb(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(100);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a >> SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shr_cross_limb);

// Large SHIFT (most bits shifted out)
static void bm_uint256_shr_large(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(200);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a >> SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shr_large);

// Shift by 256+ (EVM semantics: returns 0)
static void bm_uint256_shr_overflow(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(300);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a >> SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shr_overflow);

// Shift by 1 (common operation)
static void bm_uint256_shr_one(benchmark::State& state) {
  const auto a = random_uint256();
  constexpr auto SHIFT = Uint256(1);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a >> SHIFT;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_shr_one);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
