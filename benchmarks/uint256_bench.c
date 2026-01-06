// Benchmarks for uint256 arithmetic operations
// Covers basic operations, signed arithmetic, modular arithmetic, and exponentiation
// Methodology matches div0_cpp: constant operands computed once before benchmark loop

#include "bench.h"
#include "div0/types/uint256.h"

#include <stdint.h>
#include <stdio.h>

// Fixed seed for reproducibility
enum { BENCH_SEED = 42 };

// Simple PRNG (xorshift64)
static uint64_t prng_state = BENCH_SEED;

static uint64_t xorshift64(void) {
  uint64_t x = prng_state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  prng_state = x;
  return x;
}

static uint64_t random_u64(void) { return xorshift64(); }

static uint256_t random_uint256(void) {
  return uint256_from_limbs(xorshift64(), xorshift64(), xorshift64(), xorshift64());
}

// Helper: create a smaller divisor for realistic division benchmarks
static uint256_t make_divisor(void) {
  // Use a 2-limb value to ensure multi-limb division path
  return uint256_from_limbs(xorshift64() | 1, xorshift64(), 0, 0);
}

// Helper: negate a uint256 (two's complement)
static uint256_t negate(const uint256_t value) { return uint256_sub(uint256_zero(), value); }

// Helper: MAX uint256 value
static uint256_t uint256_max(void) { return uint256_from_limbs(~0ULL, ~0ULL, ~0ULL, ~0ULL); }

static void reset_prng(void) { prng_state = BENCH_SEED; }

// =============================================================================
// Addition Benchmarks
// =============================================================================

static void bench_add(void) {
  const uint256_t a = random_uint256();
  const uint256_t b = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_add", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_add(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_add_max_carry(void) {
  // Worst case: all 1s + 1 = carry propagates through all limbs
  const uint256_t a = uint256_max();
  const uint256_t b = uint256_from_u64(1);
  uint256_t result;

  BENCH_RUN("uint256_add (max carry)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_add(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_add_small(void) {
  // Both operands fit in single limb - no carry between limbs
  const uint256_t a = uint256_from_u64(random_u64());
  const uint256_t b = uint256_from_u64(random_u64());
  uint256_t result;

  BENCH_RUN("uint256_add (small)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_add(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// Subtraction Benchmarks
// =============================================================================

static void bench_sub(void) {
  const uint256_t a = random_uint256();
  const uint256_t b = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_sub", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_sub(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_sub_max_borrow(void) {
  // Worst case: 0 - 1 = borrow propagates through all limbs
  const uint256_t a = uint256_zero();
  const uint256_t b = uint256_from_u64(1);
  uint256_t result;

  BENCH_RUN("uint256_sub (max borrow)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_sub(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// Multiplication Benchmarks
// =============================================================================

static void bench_mul(void) {
  const uint256_t a = random_uint256();
  const uint256_t b = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_mul", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mul(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_mul_small(void) {
  // 256-bit * 64-bit (common for gas calculations)
  const uint256_t a = random_uint256();
  const uint256_t b = uint256_from_u64(random_u64());
  uint256_t result;

  BENCH_RUN("uint256_mul (256x64)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mul(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_mul_single_limb(void) {
  // Both operands fit in single limb
  const uint256_t a = uint256_from_u64(random_u64());
  const uint256_t b = uint256_from_u64(random_u64());
  uint256_t result;

  BENCH_RUN("uint256_mul (64x64)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mul(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_mul_square(void) {
  // a * a - sometimes optimizable
  const uint256_t a = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_mul (square)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mul(a, a);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// Division Benchmarks
// =============================================================================

static void bench_div(void) {
  const uint256_t a = random_uint256();
  const uint256_t b = make_divisor();
  uint256_t result;

  BENCH_RUN("uint256_div", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_div(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_div_small(void) {
  // 256-bit / 64-bit (single-limb divisor fast path)
  const uint256_t a = random_uint256();
  const uint256_t b = uint256_from_u64(random_u64() | 1);
  uint256_t result;

  BENCH_RUN("uint256_div (256/64)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_div(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_div_both_small(void) {
  // Both operands 64-bit - native hardware division
  const uint256_t a = uint256_from_u64(random_u64());
  const uint256_t b = uint256_from_u64(random_u64() | 1);
  uint256_t result;

  BENCH_RUN("uint256_div (64/64)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_div(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_div_wei_to_ether(void) {
  // Common Ethereum operation: divide by 10^18
  const uint256_t a = random_uint256();
  const uint256_t b = uint256_from_u64(1000000000000000000ULL); // 10^18
  uint256_t result;

  BENCH_RUN("uint256_div (wei->ether)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_div(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// Modulo Benchmarks
// =============================================================================

static void bench_mod(void) {
  const uint256_t a = random_uint256();
  const uint256_t b = make_divisor();
  uint256_t result;

  BENCH_RUN("uint256_mod", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mod(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_mod_small(void) {
  // 256-bit % 64-bit
  const uint256_t a = random_uint256();
  const uint256_t b = uint256_from_u64(random_u64() | 1);
  uint256_t result;

  BENCH_RUN("uint256_mod (256%64)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mod(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_mod_both_small(void) {
  // Both operands 64-bit - native hardware modulo
  const uint256_t a = uint256_from_u64(random_u64());
  const uint256_t b = uint256_from_u64(random_u64() | 1);
  uint256_t result;

  BENCH_RUN("uint256_mod (64%64)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mod(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_mod_power_of_2(void) {
  // Power of 2 modulo (could be optimized to AND)
  const uint256_t a = random_uint256();
  const uint256_t b = uint256_from_u64(1ULL << 32); // 2^32
  uint256_t result;

  BENCH_RUN("uint256_mod (pow2)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mod(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// ADDMOD Benchmarks (257-bit intermediate)
// =============================================================================

static void bench_addmod(void) {
  const uint256_t a = random_uint256();
  const uint256_t b = random_uint256();
  const uint256_t n = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_addmod", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_addmod(a, b, n);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_addmod_overflow(void) {
  // MAX + MAX exercises 257-bit intermediate path
  const uint256_t a = uint256_max();
  const uint256_t b = uint256_max();
  const uint256_t n = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_addmod (overflow)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_addmod(a, b, n);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_addmod_small_mod(void) {
  // Small modulus - tests single-limb division path
  const uint256_t a = random_uint256();
  const uint256_t b = random_uint256();
  const uint256_t n = uint256_from_u64(random_u64() | 1);
  uint256_t result;

  BENCH_RUN("uint256_addmod (small mod)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_addmod(a, b, n);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// MULMOD Benchmarks (512-bit intermediate)
// =============================================================================

static void bench_mulmod(void) {
  const uint256_t a = random_uint256();
  const uint256_t b = random_uint256();
  const uint256_t n = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_mulmod", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mulmod(a, b, n);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_mulmod_max(void) {
  // MAX * MAX exercises worst-case 512-bit intermediate
  const uint256_t a = uint256_max();
  const uint256_t b = uint256_max();
  const uint256_t n = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_mulmod (max)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mulmod(a, b, n);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_mulmod_small_product(void) {
  // Product fits in 256 bits - potential fast path
  const uint256_t a = uint256_from_u64(random_u64());
  const uint256_t b = uint256_from_u64(random_u64());
  const uint256_t n = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_mulmod (small product)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mulmod(a, b, n);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_mulmod_small_mod(void) {
  // Small modulus - single-limb division fast path
  const uint256_t a = random_uint256();
  const uint256_t b = random_uint256();
  const uint256_t n = uint256_from_u64(random_u64() | 1);
  uint256_t result;

  BENCH_RUN("uint256_mulmod (small mod)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_mulmod(a, b, n);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// Exponentiation Benchmarks
// =============================================================================

static void bench_exp_small(void) {
  const uint256_t base = random_uint256();
  const uint256_t exp = uint256_from_u64(32);
  uint256_t result;

  BENCH_RUN("uint256_exp (exp=32)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_exp(base, exp);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_exp_medium(void) {
  const uint256_t base = uint256_from_u64(3);
  const uint256_t exp = uint256_from_u64(256);
  uint256_t result;

  BENCH_RUN("uint256_exp (base=3, exp=256)", BENCH_DEFAULT_ITERATIONS / 10, {
    result = uint256_exp(base, exp);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_exp_large(void) {
  // Large exponent - many iterations in binary exponentiation
  const uint256_t base = uint256_from_u64(2);
  const uint256_t exp = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_exp (large exp)", BENCH_DEFAULT_ITERATIONS / 100, {
    result = uint256_exp(base, exp);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_exp_power_of_2(void) {
  const uint256_t base = uint256_from_u64(2);
  const uint256_t exp = uint256_from_u64(200);
  uint256_t result;

  BENCH_RUN("uint256_exp (base=2, exp=200)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_exp(base, exp);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// SDIV Benchmarks (signed division)
// =============================================================================

static void bench_sdiv(void) {
  const uint256_t a = random_uint256();
  const uint256_t b = make_divisor();
  uint256_t result;

  BENCH_RUN("uint256_sdiv", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_sdiv(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_sdiv_negative(void) {
  // Negative dividend - tests negate overhead
  const uint256_t a = negate(random_uint256());
  const uint256_t b = make_divisor();
  uint256_t result;

  BENCH_RUN("uint256_sdiv (neg dividend)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_sdiv(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_sdiv_small(void) {
  // Both fit in 64-bit positive range
  const uint256_t a = uint256_from_u64(random_u64() >> 1);
  const uint256_t b = uint256_from_u64((random_u64() >> 1) | 1);
  uint256_t result;

  BENCH_RUN("uint256_sdiv (small pos)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_sdiv(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_sdiv_both_negative(void) {
  // Both operands negative
  const uint256_t a = negate(random_uint256());
  const uint256_t b = negate(make_divisor());
  uint256_t result;

  BENCH_RUN("uint256_sdiv (both neg)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_sdiv(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_sdiv_small_negative(void) {
  // Small negative values - both fit in 64-bit
  const uint256_t a = negate(uint256_from_u64(random_u64() >> 1));
  const uint256_t b = negate(uint256_from_u64((random_u64() >> 1) | 1));
  uint256_t result;

  BENCH_RUN("uint256_sdiv (small neg)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_sdiv(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// SMOD Benchmarks (signed modulo)
// =============================================================================

static void bench_smod(void) {
  const uint256_t a = random_uint256();
  const uint256_t b = make_divisor();
  uint256_t result;

  BENCH_RUN("uint256_smod", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_smod(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_smod_small(void) {
  // Both fit in 64-bit positive range
  const uint256_t a = uint256_from_u64(random_u64() >> 1);
  const uint256_t b = uint256_from_u64((random_u64() >> 1) | 1);
  uint256_t result;

  BENCH_RUN("uint256_smod (small)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_smod(a, b);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// SIGNEXTEND Benchmarks
// =============================================================================

static void bench_signextend_byte0(void) {
  // Extend from byte 0 (8-bit to 256-bit) - most work
  const uint256_t byte_pos = uint256_from_u64(0);
  const uint256_t value = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_signextend (byte 0)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_signextend(byte_pos, value);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_signextend_byte15(void) {
  // Extend from byte 15 (128-bit to 256-bit)
  const uint256_t byte_pos = uint256_from_u64(15);
  const uint256_t value = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_signextend (byte 15)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_signextend(byte_pos, value);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

static void bench_signextend_noop(void) {
  // byte_pos >= 31 is a no-op
  const uint256_t byte_pos = uint256_from_u64(31);
  const uint256_t value = random_uint256();
  uint256_t result;

  BENCH_RUN("uint256_signextend (noop)", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_signextend(byte_pos, value);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// Utility Benchmarks
// =============================================================================

static void bench_byte_length(void) {
  const uint256_t value = random_uint256();
  size_t result;

  BENCH_RUN("uint256_byte_length", BENCH_DEFAULT_ITERATIONS, {
    result = uint256_byte_length(value);
    BENCH_DO_NOT_OPTIMIZE(result);
  });
}

// =============================================================================
// Main
// =============================================================================

int main(void) {
  printf("div0 uint256 Benchmarks\n");
  printf("========================\n");
  printf("Iterations: %d (unless noted)\n", BENCH_DEFAULT_ITERATIONS);

  // Addition
  reset_prng();
  bench_section("Addition");
  bench_add();
  bench_add_max_carry();
  bench_add_small();

  // Subtraction
  reset_prng();
  bench_section("Subtraction");
  bench_sub();
  bench_sub_max_borrow();

  // Multiplication
  reset_prng();
  bench_section("Multiplication");
  bench_mul();
  bench_mul_small();
  bench_mul_single_limb();
  bench_mul_square();

  // Division
  reset_prng();
  bench_section("Division");
  bench_div();
  bench_div_small();
  bench_div_both_small();
  bench_div_wei_to_ether();

  // Modulo
  reset_prng();
  bench_section("Modulo");
  bench_mod();
  bench_mod_small();
  bench_mod_both_small();
  bench_mod_power_of_2();

  // ADDMOD
  reset_prng();
  bench_section("ADDMOD (257-bit intermediate)");
  bench_addmod();
  bench_addmod_overflow();
  bench_addmod_small_mod();

  // MULMOD
  reset_prng();
  bench_section("MULMOD (512-bit intermediate)");
  bench_mulmod();
  bench_mulmod_max();
  bench_mulmod_small_product();
  bench_mulmod_small_mod();

  // Exponentiation
  reset_prng();
  bench_section("Exponentiation");
  bench_exp_small();
  bench_exp_medium();
  bench_exp_large();
  bench_exp_power_of_2();

  // SDIV
  reset_prng();
  bench_section("SDIV (signed division)");
  bench_sdiv();
  bench_sdiv_negative();
  bench_sdiv_small();
  bench_sdiv_both_negative();
  bench_sdiv_small_negative();

  // SMOD
  reset_prng();
  bench_section("SMOD (signed modulo)");
  bench_smod();
  bench_smod_small();

  // SIGNEXTEND
  reset_prng();
  bench_section("SIGNEXTEND");
  bench_signextend_byte0();
  bench_signextend_byte15();
  bench_signextend_noop();

  // Utility
  reset_prng();
  bench_section("Utility");
  bench_byte_length();

  printf("\nBenchmarks complete.\n");
  return 0;
}
