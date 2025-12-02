#include <benchmark/benchmark.h>

#include <random>

#include "div0/types/uint256.h"

using namespace div0::types;

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp, *-avoid-non-const-global-variables)

static Uint256 random_uint256() { return Uint256(rng(), rng(), rng(), rng()); }

static uint64_t random_uint64() { return rng(); }

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// ============================================================================
// Addition Benchmarks
// ============================================================================

static void bm_uint256_add(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a + b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_add);  // NOLINT(*-owning-memory, *-avoid-non-const-global-variables)

// Addition with carry propagation (worst case: all 1s)
static void bm_uint256_add_max_carry(benchmark::State& state) {
  const Uint256 a = ~Uint256(0);
  const Uint256 b(1);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a + b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_add_max_carry);

// Addition with small values (no carry)
static void bm_uint256_add_small(benchmark::State& state) {
  const Uint256 a(random_uint64());
  const Uint256 b(random_uint64());

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a + b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_add_small);

// ============================================================================
// Subtraction Benchmarks
// ============================================================================

static void bm_uint256_sub(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a - b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_sub);

// Subtraction with borrow propagation (worst case)
static void bm_uint256_sub_max_borrow(benchmark::State& state) {
  const Uint256 a(0);
  const Uint256 b(1);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a - b;  // Wraps to all 1s
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_sub_max_borrow);

// ============================================================================
// Multiplication Benchmarks
// ============================================================================

static void bm_uint256_mul(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a * b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mul);

// Multiply by small value (common in EVM for gas calculations, etc.)
static void bm_uint256_mul_small(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b(random_uint64());

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a * b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mul_small);

// Square (a * a) - sometimes optimizable
static void bm_uint256_square(benchmark::State& state) {
  const Uint256 a = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a * a;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_square);

// ============================================================================
// Division Benchmarks
// ============================================================================

// Helper to ensure dividend > divisor (shifts divisor right)
static Uint256 smaller_than(const Uint256& dividend) {
  // Shift right by random amount (1-128 bits) to guarantee smaller
  const unsigned shift = (rng() % 128) + 1;
  return dividend >> Uint256(shift);
}

// Multi-limb divisor (Knuth Algorithm D path in most cases)
static void bm_uint256_div(benchmark::State& state) {
  const Uint256 a = random_uint256();
  // Use a realistic multi-limb divisor to avoid early exit because a < b
  const Uint256 b = smaller_than(a);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a / b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_div);

// Single-limb divisor fast path (256-bit dividend / 64-bit divisor)
static void bm_uint256_div_small(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b(random_uint64());

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a / b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_div_small);

// Both operands 64-bit - native hardware division fast path
static void bm_uint256_div_both_small(benchmark::State& state) {
  rng.seed(42);
  const Uint256 a(random_uint64());
  const Uint256 b(random_uint64() | 1);  // Ensure non-zero

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a / b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_div_both_small);

// Wei to Ether: divide by 10^18
static void bm_uint256_div_wei_to_ether(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b(1000000000000000000ULL);  // 10^18

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a / b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_div_wei_to_ether);

// ============================================================================
// Modulo Benchmarks
// ============================================================================

// Multi-limb divisor
static void bm_uint256_mod(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = smaller_than(a);

  for ([[maybe_unused]] auto _ : state) {
    auto result = a % b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mod);

// Single-limb divisor fast path (256-bit dividend % 64-bit divisor)
static void bm_uint256_mod_small(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = Uint256(random_uint64());

  for ([[maybe_unused]] auto _ : state) {
    auto result = a % b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mod_small);

// Both operands 64-bit - native hardware modulo fast path
static void bm_uint256_mod_both_small(benchmark::State& state) {
  rng.seed(42);
  const Uint256 a(random_uint64());
  const Uint256 b(random_uint64() | 1);  // Ensure non-zero

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = a % b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mod_both_small);

// Power of 2 modulo (can be optimized to AND)
static void bm_uint256_mod_power_of2(benchmark::State& state) {
  const auto a = random_uint256();
  const auto b = Uint256(1ULL << 32);  // 2^32

  for ([[maybe_unused]] auto _ : state) {
    auto result = a % b;
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mod_power_of2);

// ============================================================================
// ADDMOD Benchmarks (257-bit intermediate)
// ============================================================================

// Random values - typical case
static void bm_uint256_addmod(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();
  const Uint256 m = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::addmod(a, b, m);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_addmod);

// Overflow case - sum exceeds 256 bits (exercises 257-bit path)
static void bm_uint256_addmod_overflow(benchmark::State& state) {
  const Uint256 a = ~Uint256(0);  // MAX256
  const Uint256 b = ~Uint256(0);  // MAX256
  const Uint256 m = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::addmod(a, b, m);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_addmod_overflow);

// Small modulus - tests division performance
static void bm_uint256_addmod_small_mod(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();
  const Uint256 m(random_uint64());

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::addmod(a, b, m);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_addmod_small_mod);

// ============================================================================
// MULMOD Benchmarks (512-bit intermediate)
// ============================================================================

// Random values - typical case with 512-bit product
static void bm_uint256_mulmod(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();
  const Uint256 m = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::mulmod(a, b, m);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mulmod);

// MAX * MAX - worst case for 512-bit multiplication
static void bm_uint256_mulmod_max(benchmark::State& state) {
  const Uint256 a = ~Uint256(0);  // MAX256
  const Uint256 b = ~Uint256(0);  // MAX256
  const Uint256 m = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::mulmod(a, b, m);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mulmod_max);

// Product fits in 256 bits - fast path
static void bm_uint256_mulmod_small_product(benchmark::State& state) {
  const Uint256 a(random_uint64());
  const Uint256 b(random_uint64());
  const Uint256 m = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::mulmod(a, b, m);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mulmod_small_product);

// Small modulus - single-limb division fast path
static void bm_uint256_mulmod_small_mod(benchmark::State& state) {
  rng.seed(42);  // Reset seed for consistent test values
  const Uint256 a = random_uint256();
  const Uint256 b = random_uint256();
  const Uint256 m(random_uint64());

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::mulmod(a, b, m);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_mulmod_small_mod);

// ============================================================================
// EXP Benchmarks (binary exponentiation)
// ============================================================================

// Small exponent (common case)
static void bm_uint256_exp_small(benchmark::State& state) {
  const Uint256 base = random_uint256();
  const Uint256 exp(32);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::exp(base, exp);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_exp_small);

// Medium exponent
static void bm_uint256_exp_medium(benchmark::State& state) {
  const Uint256 base(3);
  const Uint256 exp(256);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::exp(base, exp);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_exp_medium);

// Large exponent - many iterations
static void bm_uint256_exp_large(benchmark::State& state) {
  const Uint256 base(2);
  const Uint256 exp = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::exp(base, exp);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_exp_large);

// Power of 2 base - may have optimization opportunity
static void bm_uint256_exp_power_of_2(benchmark::State& state) {
  const Uint256 base(2);
  const Uint256 exp(200);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::exp(base, exp);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_exp_power_of_2);

// ============================================================================
// SDIV Benchmarks (signed division)
// ============================================================================

static void bm_uint256_sdiv(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = smaller_than(a);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::sdiv(a, b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_sdiv);

// Negative dividend - tests negate overhead
static void bm_uint256_sdiv_negative(benchmark::State& state) {
  const Uint256 a = random_uint256().negate();  // Make negative
  const Uint256 b = smaller_than(random_uint256());

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::sdiv(a, b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_sdiv_negative);

// Small values - both fit in 64-bit (optimization target)
static void bm_uint256_sdiv_small(benchmark::State& state) {
  rng.seed(42);
  const Uint256 a(random_uint64() >> 1);        // Positive 63-bit value
  const Uint256 b((random_uint64() >> 1) | 1);  // Ensure non-zero

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::sdiv(a, b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_sdiv_small);

// Both operands negative
static void bm_uint256_sdiv_both_negative(benchmark::State& state) {
  rng.seed(42);
  const Uint256 a = random_uint256().negate();
  const Uint256 b = smaller_than(random_uint256()).negate();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::sdiv(a, b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_sdiv_both_negative);

// Small negative values - both fit in 64-bit
static void bm_uint256_sdiv_small_negative(benchmark::State& state) {
  rng.seed(42);
  const Uint256 a = Uint256(random_uint64() >> 1).negate();
  const Uint256 b = Uint256((random_uint64() >> 1) | 1).negate();

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::sdiv(a, b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_sdiv_small_negative);

// ============================================================================
// SMOD Benchmarks (signed modulo)
// ============================================================================

static void bm_uint256_smod(benchmark::State& state) {
  const Uint256 a = random_uint256();
  const Uint256 b = smaller_than(a);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::smod(a, b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_smod);

// Small values - both fit in 64-bit
static void bm_uint256_smod_small(benchmark::State& state) {
  rng.seed(42);
  const Uint256 a(random_uint64() >> 1);
  const Uint256 b((random_uint64() >> 1) | 1);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::smod(a, b);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_smod_small);

// ============================================================================
// SIGNEXTEND Benchmarks
// ============================================================================

// Extend from byte 0 (8-bit to 256-bit)
static void bm_uint256_signextend_byte0(benchmark::State& state) {
  const Uint256 x = random_uint256();
  const Uint256 byte_pos(0);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::signextend(byte_pos, x);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_signextend_byte0);

// Extend from byte 15 (128-bit to 256-bit)
static void bm_uint256_signextend_byte15(benchmark::State& state) {
  const Uint256 x = random_uint256();
  const Uint256 byte_pos(15);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::signextend(byte_pos, x);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_signextend_byte15);

// byte_pos >= 31 - no-op fast path
static void bm_uint256_signextend_noop(benchmark::State& state) {
  const Uint256 x = random_uint256();
  const Uint256 byte_pos(31);

  for ([[maybe_unused]] auto _ : state) {
    Uint256 result = Uint256::signextend(byte_pos, x);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_uint256_signextend_noop);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
