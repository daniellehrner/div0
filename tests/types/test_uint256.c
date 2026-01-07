#include "test_uint256.h"

#include "div0/types/uint256.h"

#include "unity.h"

void test_uint256_zero_is_zero(void) {
  uint256_t z = uint256_zero();
  TEST_ASSERT_TRUE(uint256_is_zero(z));
}

void test_uint256_from_u64_works(void) {
  uint256_t a = uint256_from_u64(42);
  TEST_ASSERT_FALSE(uint256_is_zero(a));
  TEST_ASSERT_EQUAL_UINT64(42, a.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, a.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(0, a.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0, a.limbs[3]);
}

void test_uint256_eq_works(void) {
  uint256_t a = uint256_from_u64(123);
  uint256_t b = uint256_from_u64(123);
  uint256_t c = uint256_from_u64(456);
  TEST_ASSERT_TRUE(uint256_eq(a, b));
  TEST_ASSERT_FALSE(uint256_eq(a, c));
}

void test_uint256_add_no_overflow(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(200);
  uint256_t result = uint256_add(a, b);
  TEST_ASSERT_EQUAL_UINT64(300, result.limbs[0]);
}

void test_uint256_add_with_carry(void) {
  // Test carry propagation: (2^64 - 1) + 1 = 2^64
  uint256_t a = uint256_zero();
  a.limbs[0] = UINT64_MAX;
  uint256_t b = uint256_from_u64(1);
  uint256_t result = uint256_add(a, b);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[1]);
}

void test_uint256_add_overflow_wraps(void) {
  // Max uint256 + 1 should wrap to 0
  uint256_t max;
  max.limbs[0] = UINT64_MAX;
  max.limbs[1] = UINT64_MAX;
  max.limbs[2] = UINT64_MAX;
  max.limbs[3] = UINT64_MAX;
  uint256_t one = uint256_from_u64(1);
  uint256_t result = uint256_add(max, one);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_bytes_be_roundtrip(void) {
  // Create a value with bytes in all limbs
  uint256_t original = uint256_zero();
  original.limbs[0] = 0x0807060504030201ULL;
  original.limbs[1] = 0x100F0E0D0C0B0A09ULL;
  original.limbs[2] = 0x1817161514131211ULL;
  original.limbs[3] = 0x201F1E1D1C1B1A19ULL;

  uint8_t bytes[32];
  uint256_to_bytes_be(original, bytes);

  uint256_t restored = uint256_from_bytes_be(bytes, 32);
  TEST_ASSERT_TRUE(uint256_eq(original, restored));
}

void test_uint256_from_bytes_be_short(void) {
  // 2 bytes: 0x0102 = 258
  uint8_t bytes[] = {0x01, 0x02};
  uint256_t value = uint256_from_bytes_be(bytes, 2);
  TEST_ASSERT_EQUAL_UINT64(258, value.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, value.limbs[1]);
}

void test_uint256_sub_basic(void) {
  uint256_t a = uint256_from_u64(200);
  uint256_t b = uint256_from_u64(100);
  uint256_t result = uint256_sub(a, b);
  TEST_ASSERT_EQUAL_UINT64(100, result.limbs[0]);
}

void test_uint256_sub_with_borrow(void) {
  // 2^64 - 1 = result with borrow from second limb
  uint256_t a = uint256_from_limbs(0, 1, 0, 0); // 2^64
  uint256_t b = uint256_from_u64(1);
  uint256_t result = uint256_sub(a, b);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
}

void test_uint256_sub_underflow_wraps(void) {
  // 0 - 1 should wrap to MAX_UINT256
  uint256_t zero = uint256_zero();
  uint256_t one = uint256_from_u64(1);
  uint256_t result = uint256_sub(zero, one);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[3]);
}

void test_uint256_lt_basic(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(200);
  TEST_ASSERT_TRUE(uint256_lt(a, b));
  TEST_ASSERT_FALSE(uint256_lt(b, a));
}

void test_uint256_lt_equal(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(100);
  TEST_ASSERT_FALSE(uint256_lt(a, b));
}

void test_uint256_lt_multi_limb(void) {
  // a = 2^64 (only second limb set)
  uint256_t a = uint256_from_limbs(0, 1, 0, 0);
  // b = MAX_UINT64 (only first limb set)
  uint256_t b = uint256_from_limbs(UINT64_MAX, 0, 0, 0);
  TEST_ASSERT_FALSE(uint256_lt(a, b)); // a > b
  TEST_ASSERT_TRUE(uint256_lt(b, a));  // b < a
}

void test_uint256_mul_basic(void) {
  uint256_t a = uint256_from_u64(6);
  uint256_t b = uint256_from_u64(7);
  uint256_t result = uint256_mul(a, b);
  TEST_ASSERT_EQUAL_UINT64(42, result.limbs[0]);
}

void test_uint256_mul_limb_boundary(void) {
  // 2^32 * 2^32 = 2^64
  uint256_t a = uint256_from_u64(1ULL << 32);
  uint256_t b = uint256_from_u64(1ULL << 32);
  uint256_t result = uint256_mul(a, b);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[1]);
}

void test_uint256_mul_multi_limb(void) {
  // (2^64) * (2^64) = 2^128
  uint256_t a = uint256_from_limbs(0, 1, 0, 0);
  uint256_t b = uint256_from_limbs(0, 1, 0, 0);
  uint256_t result = uint256_mul(a, b);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[3]);
}

void test_uint256_mul_overflow_wraps(void) {
  // MAX * 2 should wrap
  uint256_t max = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t two = uint256_from_u64(2);
  uint256_t result = uint256_mul(max, two);
  // MAX * 2 = 2 * (2^256 - 1) = 2^257 - 2 ≡ -2 (mod 2^256) = MAX - 1
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX - 1, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[3]);
}

void test_uint256_div_basic(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(10);
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_EQUAL_UINT64(10, result.limbs[0]);
}

void test_uint256_div_by_zero(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_zero();
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_div_smaller_than_divisor(void) {
  uint256_t a = uint256_from_u64(5);
  uint256_t b = uint256_from_u64(10);
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_div_with_remainder(void) {
  uint256_t a = uint256_from_u64(17);
  uint256_t b = uint256_from_u64(5);
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_EQUAL_UINT64(3, result.limbs[0]);
}

void test_uint256_div_multi_limb(void) {
  // 2^128 / 2^64 = 2^64
  uint256_t a = uint256_from_limbs(0, 0, 1, 0); // 2^128
  uint256_t b = uint256_from_limbs(0, 1, 0, 0); // 2^64
  uint256_t result = uint256_div(a, b);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[3]);
}

void test_uint256_mod_basic(void) {
  uint256_t a = uint256_from_u64(17);
  uint256_t b = uint256_from_u64(5);
  uint256_t result = uint256_mod(a, b);
  TEST_ASSERT_EQUAL_UINT64(2, result.limbs[0]);
}

void test_uint256_mod_by_zero(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_zero();
  uint256_t result = uint256_mod(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_mod_by_one(void) {
  uint256_t a = uint256_from_u64(12345);
  uint256_t b = uint256_from_u64(1);
  uint256_t result = uint256_mod(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_mod_no_remainder(void) {
  uint256_t a = uint256_from_u64(100);
  uint256_t b = uint256_from_u64(10);
  uint256_t result = uint256_mod(a, b);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_div_mod_consistency(void) {
  // Verify: a == (a / b) * b + (a % b)
  uint256_t a = uint256_from_u64(12345);
  uint256_t b = uint256_from_u64(67);
  uint256_t q = uint256_div(a, b);
  uint256_t r = uint256_mod(a, b);
  uint256_t reconstructed = uint256_add(uint256_mul(q, b), r);
  TEST_ASSERT_TRUE(uint256_eq(a, reconstructed));
}

// =============================================================================
// Signed Arithmetic Tests
// =============================================================================

// Helper to create -1 (all bits set)
static uint256_t uint256_minus_one(void) {
  return uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
}

// Helper to negate a value (two's complement)
static uint256_t uint256_negate(uint256_t a) {
  // -a = ~a + 1
  uint256_t not_a = uint256_from_limbs(~a.limbs[0], ~a.limbs[1], ~a.limbs[2], ~a.limbs[3]);
  return uint256_add(not_a, uint256_from_u64(1));
}

void test_uint256_is_negative_zero(void) {
  TEST_ASSERT_FALSE(uint256_is_negative(uint256_zero()));
}

void test_uint256_is_negative_positive(void) {
  TEST_ASSERT_FALSE(uint256_is_negative(uint256_from_u64(1)));
  TEST_ASSERT_FALSE(uint256_is_negative(uint256_from_u64(UINT64_MAX)));
  // Max positive: MSB is 0
  uint256_t max_positive = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX >> 1);
  TEST_ASSERT_FALSE(uint256_is_negative(max_positive));
}

void test_uint256_is_negative_negative(void) {
  TEST_ASSERT_TRUE(uint256_is_negative(uint256_minus_one()));
  // Min negative: -2^255
  uint256_t min_negative = uint256_from_limbs(0, 0, 0, 0x8000000000000000ULL);
  TEST_ASSERT_TRUE(uint256_is_negative(min_negative));
}

void test_uint256_sdiv_by_zero(void) {
  // EVM spec: SDIV by zero returns 0
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_sdiv(uint256_zero(), uint256_zero())));
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_sdiv(uint256_from_u64(1), uint256_zero())));
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_sdiv(uint256_minus_one(), uint256_zero())));
}

void test_uint256_sdiv_positive_by_positive(void) {
  // 10 / 3 = 3
  uint256_t result = uint256_sdiv(uint256_from_u64(10), uint256_from_u64(3));
  TEST_ASSERT_EQUAL_UINT64(3, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);

  // 100 / 10 = 10
  result = uint256_sdiv(uint256_from_u64(100), uint256_from_u64(10));
  TEST_ASSERT_EQUAL_UINT64(10, result.limbs[0]);
}

void test_uint256_sdiv_negative_by_positive(void) {
  // -10 / 3 = -3 (truncated toward zero)
  uint256_t neg_10 = uint256_negate(uint256_from_u64(10));
  uint256_t neg_3 = uint256_negate(uint256_from_u64(3));
  uint256_t result = uint256_sdiv(neg_10, uint256_from_u64(3));
  TEST_ASSERT_TRUE(uint256_eq(result, neg_3));
}

void test_uint256_sdiv_positive_by_negative(void) {
  // 10 / -3 = -3 (truncated toward zero)
  uint256_t neg_3 = uint256_negate(uint256_from_u64(3));
  uint256_t result = uint256_sdiv(uint256_from_u64(10), neg_3);
  TEST_ASSERT_TRUE(uint256_eq(result, neg_3));
}

void test_uint256_sdiv_negative_by_negative(void) {
  // -10 / -3 = 3 (positive result)
  uint256_t neg_10 = uint256_negate(uint256_from_u64(10));
  uint256_t neg_3 = uint256_negate(uint256_from_u64(3));
  uint256_t result = uint256_sdiv(neg_10, neg_3);
  TEST_ASSERT_EQUAL_UINT64(3, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
}

void test_uint256_sdiv_min_value_by_minus_one(void) {
  // EVM special case: MIN_VALUE / -1 = MIN_VALUE (overflow)
  uint256_t min_negative = uint256_from_limbs(0, 0, 0, 0x8000000000000000ULL);
  uint256_t result = uint256_sdiv(min_negative, uint256_minus_one());
  TEST_ASSERT_TRUE(uint256_eq(result, min_negative));
}

void test_uint256_smod_by_zero(void) {
  // EVM spec: SMOD by zero returns 0
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_smod(uint256_zero(), uint256_zero())));
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_smod(uint256_from_u64(1), uint256_zero())));
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_smod(uint256_minus_one(), uint256_zero())));
}

void test_uint256_smod_positive_by_positive(void) {
  // 10 % 3 = 1
  uint256_t result = uint256_smod(uint256_from_u64(10), uint256_from_u64(3));
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);

  // 100 % 10 = 0
  result = uint256_smod(uint256_from_u64(100), uint256_from_u64(10));
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_smod_negative_by_positive(void) {
  // -10 % 3 = -1 (sign follows dividend)
  uint256_t neg_10 = uint256_negate(uint256_from_u64(10));
  uint256_t neg_1 = uint256_negate(uint256_from_u64(1));
  uint256_t result = uint256_smod(neg_10, uint256_from_u64(3));
  TEST_ASSERT_TRUE(uint256_eq(result, neg_1));
}

void test_uint256_smod_positive_by_negative(void) {
  // 10 % -3 = 1 (sign follows dividend, which is positive)
  uint256_t neg_3 = uint256_negate(uint256_from_u64(3));
  uint256_t result = uint256_smod(uint256_from_u64(10), neg_3);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
}

void test_uint256_smod_negative_by_negative(void) {
  // -10 % -3 = -1 (sign follows dividend)
  uint256_t neg_10 = uint256_negate(uint256_from_u64(10));
  uint256_t neg_3 = uint256_negate(uint256_from_u64(3));
  uint256_t neg_1 = uint256_negate(uint256_from_u64(1));
  uint256_t result = uint256_smod(neg_10, neg_3);
  TEST_ASSERT_TRUE(uint256_eq(result, neg_1));
}

void test_uint256_sdiv_smod_identity(void) {
  // For signed: a = (a SDIV b) * b + (a SMOD b)
  uint256_t neg_10 = uint256_negate(uint256_from_u64(10));
  uint256_t three = uint256_from_u64(3);

  uint256_t q = uint256_sdiv(neg_10, three);
  uint256_t r = uint256_smod(neg_10, three);

  // Verify: a = q * b + r
  uint256_t reconstructed = uint256_add(uint256_mul(q, three), r);
  TEST_ASSERT_TRUE(uint256_eq(neg_10, reconstructed));
}

// =============================================================================
// Sign Extend Tests
// =============================================================================

void test_uint256_signextend_byte_pos_zero_positive(void) {
  // byte_pos = 0 means extend from bit 7
  // 0x7F has bit 7 = 0 (positive), should remain unchanged in low byte
  uint256_t result = uint256_signextend(uint256_zero(), uint256_from_u64(0x7F));
  TEST_ASSERT_EQUAL_UINT64(0x7F, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
}

void test_uint256_signextend_byte_pos_zero_negative(void) {
  // 0x80 has bit 7 = 1 (negative), should extend 1s
  uint256_t result = uint256_signextend(uint256_zero(), uint256_from_u64(0x80));
  // Result should be 0xFFFFFFFF...FFFFFF80
  TEST_ASSERT_TRUE(uint256_is_negative(result));
  TEST_ASSERT_EQUAL_UINT64(0xFFFFFFFFFFFFFF80ULL, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, result.limbs[3]);
}

void test_uint256_signextend_byte_pos_one_positive(void) {
  // byte_pos = 1 means extend from bit 15
  // 0x7FFF has bit 15 = 0 (positive)
  uint256_t result = uint256_signextend(uint256_from_u64(1), uint256_from_u64(0x7FFF));
  TEST_ASSERT_EQUAL_UINT64(0x7FFF, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
}

void test_uint256_signextend_byte_pos_one_negative(void) {
  // 0x8000 has bit 15 = 1 (negative), should extend 1s
  uint256_t result = uint256_signextend(uint256_from_u64(1), uint256_from_u64(0x8000));
  // Result should be 0xFFFF...FFFF8000
  TEST_ASSERT_TRUE(uint256_is_negative(result));
  TEST_ASSERT_EQUAL_UINT64(0xFFFFFFFFFFFF8000ULL, result.limbs[0]);
}

void test_uint256_signextend_byte_pos_31_or_larger(void) {
  // byte_pos >= 31 means all 256 bits are used, no extension needed
  uint256_t val = uint256_from_limbs(0x12345678, 0x9ABCDEF0, 0x11111111, 0x22222222);

  uint256_t result31 = uint256_signextend(uint256_from_u64(31), val);
  TEST_ASSERT_TRUE(uint256_eq(result31, val));

  uint256_t result100 = uint256_signextend(uint256_from_u64(100), val);
  TEST_ASSERT_TRUE(uint256_eq(result100, val));
}

void test_uint256_signextend_clears_high_bits(void) {
  // When sign bit is 0, high bits should be cleared
  uint256_t val = uint256_from_limbs(0x0000007F, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t result = uint256_signextend(uint256_zero(), val);
  TEST_ASSERT_EQUAL_UINT64(0x7F, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[3]);
}

// =============================================================================
// Modular Arithmetic Tests
// =============================================================================

void test_uint256_addmod_by_zero(void) {
  // EVM spec: ADDMOD with N=0 returns 0
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_addmod(uint256_zero(), uint256_zero(), uint256_zero())));
  TEST_ASSERT_TRUE(
      uint256_is_zero(uint256_addmod(uint256_from_u64(1), uint256_from_u64(1), uint256_zero())));
}

void test_uint256_addmod_no_overflow(void) {
  // Simple cases without overflow
  uint256_t result = uint256_addmod(uint256_from_u64(5), uint256_from_u64(3), uint256_from_u64(10));
  TEST_ASSERT_EQUAL_UINT64(8, result.limbs[0]);

  result = uint256_addmod(uint256_from_u64(7), uint256_from_u64(8), uint256_from_u64(10));
  TEST_ASSERT_EQUAL_UINT64(5, result.limbs[0]);
}

void test_uint256_addmod_with_overflow(void) {
  // MAX256 + 1 mod MAX256 = 1
  uint256_t max256 = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t result = uint256_addmod(max256, uint256_from_u64(1), max256);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);

  // MAX256 + MAX256 mod MAX256 = 0
  result = uint256_addmod(max256, max256, max256);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_addmod_result_equals_modulus(void) {
  // a + b = N exactly
  uint256_t result = uint256_addmod(uint256_from_u64(5), uint256_from_u64(5), uint256_from_u64(10));
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_addmod_modulus_one(void) {
  // Any sum mod 1 = 0
  uint256_t result =
      uint256_addmod(uint256_from_u64(12345), uint256_from_u64(67890), uint256_from_u64(1));
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_mulmod_by_zero(void) {
  // EVM spec: MULMOD with N=0 returns 0
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_mulmod(uint256_zero(), uint256_zero(), uint256_zero())));
  TEST_ASSERT_TRUE(
      uint256_is_zero(uint256_mulmod(uint256_from_u64(1), uint256_from_u64(1), uint256_zero())));
}

void test_uint256_mulmod_no_overflow(void) {
  // Simple cases where product fits in 256 bits
  uint256_t result = uint256_mulmod(uint256_from_u64(5), uint256_from_u64(3), uint256_from_u64(10));
  TEST_ASSERT_EQUAL_UINT64(5, result.limbs[0]); // 15 % 10 = 5

  result = uint256_mulmod(uint256_from_u64(7), uint256_from_u64(8), uint256_from_u64(10));
  TEST_ASSERT_EQUAL_UINT64(6, result.limbs[0]); // 56 % 10 = 6
}

void test_uint256_mulmod_with_overflow(void) {
  // MAX256 * MAX256 mod MAX256 = 0
  uint256_t max256 = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t result = uint256_mulmod(max256, max256, max256);
  TEST_ASSERT_TRUE(uint256_is_zero(result));

  // MAX256 * 2 mod (MAX256 - 1) = 2
  uint256_t max_minus_1 = uint256_sub(max256, uint256_from_u64(1));
  result = uint256_mulmod(max256, uint256_from_u64(2), max_minus_1);
  TEST_ASSERT_EQUAL_UINT64(2, result.limbs[0]);
}

void test_uint256_mulmod_modulus_one(void) {
  // Any product mod 1 = 0
  uint256_t result =
      uint256_mulmod(uint256_from_u64(12345), uint256_from_u64(67890), uint256_from_u64(1));
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

// =============================================================================
// Exponentiation Tests
// =============================================================================

void test_uint256_exp_exponent_zero(void) {
  // x^0 = 1 for any x
  uint256_t one = uint256_from_u64(1);
  TEST_ASSERT_TRUE(uint256_eq(uint256_exp(uint256_zero(), uint256_zero()), one));
  TEST_ASSERT_TRUE(uint256_eq(uint256_exp(uint256_from_u64(1), uint256_zero()), one));
  TEST_ASSERT_TRUE(uint256_eq(uint256_exp(uint256_from_u64(12345), uint256_zero()), one));
}

void test_uint256_exp_base_zero(void) {
  // 0^n = 0 for n > 0
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_exp(uint256_zero(), uint256_from_u64(1))));
  TEST_ASSERT_TRUE(uint256_is_zero(uint256_exp(uint256_zero(), uint256_from_u64(100))));
}

void test_uint256_exp_base_one(void) {
  // 1^n = 1 for any n
  uint256_t one = uint256_from_u64(1);
  TEST_ASSERT_TRUE(uint256_eq(uint256_exp(one, one), one));
  TEST_ASSERT_TRUE(uint256_eq(uint256_exp(one, uint256_from_u64(100)), one));
}

void test_uint256_exp_exponent_one(void) {
  // x^1 = x
  uint256_t val = uint256_from_u64(42);
  TEST_ASSERT_TRUE(uint256_eq(uint256_exp(val, uint256_from_u64(1)), val));
}

void test_uint256_exp_small_powers(void) {
  // 2^10 = 1024
  uint256_t result = uint256_exp(uint256_from_u64(2), uint256_from_u64(10));
  TEST_ASSERT_EQUAL_UINT64(1024, result.limbs[0]);

  // 3^5 = 243
  result = uint256_exp(uint256_from_u64(3), uint256_from_u64(5));
  TEST_ASSERT_EQUAL_UINT64(243, result.limbs[0]);

  // 10^6 = 1,000,000
  result = uint256_exp(uint256_from_u64(10), uint256_from_u64(6));
  TEST_ASSERT_EQUAL_UINT64(1000000, result.limbs[0]);
}

void test_uint256_exp_powers_of_two(void) {
  // 2^64 = 2^64 (limb 1 = 1)
  uint256_t result = uint256_exp(uint256_from_u64(2), uint256_from_u64(64));
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[2]);

  // 2^128 (limb 2 = 1)
  result = uint256_exp(uint256_from_u64(2), uint256_from_u64(128));
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[2]);

  // 2^255 (highest bit)
  result = uint256_exp(uint256_from_u64(2), uint256_from_u64(255));
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0x8000000000000000ULL, result.limbs[3]);
}

void test_uint256_exp_overflow(void) {
  // 2^256 overflows to 0
  uint256_t result = uint256_exp(uint256_from_u64(2), uint256_from_u64(256));
  TEST_ASSERT_TRUE(uint256_is_zero(result));

  // 2^300 also 0
  result = uint256_exp(uint256_from_u64(2), uint256_from_u64(300));
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

// =============================================================================
// Byte Length Tests
// =============================================================================

void test_uint256_byte_length_zero(void) {
  TEST_ASSERT_EQUAL_UINT64(0, uint256_byte_length(uint256_zero()));
}

void test_uint256_byte_length_small_values(void) {
  TEST_ASSERT_EQUAL_UINT64(1, uint256_byte_length(uint256_from_u64(1)));
  TEST_ASSERT_EQUAL_UINT64(1, uint256_byte_length(uint256_from_u64(255)));
  TEST_ASSERT_EQUAL_UINT64(2, uint256_byte_length(uint256_from_u64(256)));
  TEST_ASSERT_EQUAL_UINT64(2, uint256_byte_length(uint256_from_u64(0xFFFF)));
  TEST_ASSERT_EQUAL_UINT64(3, uint256_byte_length(uint256_from_u64(0x10000)));
}

void test_uint256_byte_length_limb_boundaries(void) {
  // Limb 0 full
  TEST_ASSERT_EQUAL_UINT64(8, uint256_byte_length(uint256_from_limbs(UINT64_MAX, 0, 0, 0)));

  // Limb 1 starts
  TEST_ASSERT_EQUAL_UINT64(9, uint256_byte_length(uint256_from_limbs(0, 1, 0, 0)));

  // Limb 1 full
  TEST_ASSERT_EQUAL_UINT64(16,
                           uint256_byte_length(uint256_from_limbs(UINT64_MAX, UINT64_MAX, 0, 0)));

  // Limb 2 starts
  TEST_ASSERT_EQUAL_UINT64(17, uint256_byte_length(uint256_from_limbs(0, 0, 1, 0)));

  // Limb 3 starts
  TEST_ASSERT_EQUAL_UINT64(25, uint256_byte_length(uint256_from_limbs(0, 0, 0, 1)));

  // Max value
  TEST_ASSERT_EQUAL_UINT64(
      32, uint256_byte_length(uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX)));
}

// =============================================================================
// Bitwise Operation Tests
// =============================================================================

void test_uint256_and_basic(void) {
  uint256_t a = uint256_from_u64(0xFF00FF00);
  uint256_t b = uint256_from_u64(0x0FF00FF0);
  uint256_t result = uint256_and(a, b);
  TEST_ASSERT_EQUAL_UINT64(0x0F000F00, result.limbs[0]);

  // Multi-limb AND
  uint256_t c = uint256_from_limbs(0xFFFFFFFF, 0xAAAAAAAA, 0x55555555, 0x12345678);
  uint256_t d = uint256_from_limbs(0x0F0F0F0F, 0xF0F0F0F0, 0x0F0F0F0F, 0xFFFFFFFF);
  result = uint256_and(c, d);
  TEST_ASSERT_EQUAL_UINT64(0x0F0F0F0F, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0xA0A0A0A0, result.limbs[1]);
  TEST_ASSERT_EQUAL_UINT64(0x05050505, result.limbs[2]);
  TEST_ASSERT_EQUAL_UINT64(0x12345678, result.limbs[3]);
}

void test_uint256_or_basic(void) {
  uint256_t a = uint256_from_u64(0xF0F0F0F0);
  uint256_t b = uint256_from_u64(0x0F0F0F0F);
  uint256_t result = uint256_or(a, b);
  TEST_ASSERT_EQUAL_UINT64(0xFFFFFFFF, result.limbs[0]);

  // OR with zero
  result = uint256_or(a, uint256_zero());
  TEST_ASSERT_TRUE(uint256_eq(result, a));
}

void test_uint256_xor_basic(void) {
  uint256_t a = uint256_from_u64(0xFFFF0000);
  uint256_t b = uint256_from_u64(0xFF00FF00);
  uint256_t result = uint256_xor(a, b);
  TEST_ASSERT_EQUAL_UINT64(0x00FFFF00, result.limbs[0]);

  // XOR with self = 0
  result = uint256_xor(a, a);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_not_basic(void) {
  uint256_t zero = uint256_zero();
  uint256_t result = uint256_not(zero);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[0]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[1]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[2]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[3]);

  // Double NOT = identity
  uint256_t val = uint256_from_u64(0x12345678);
  result = uint256_not(uint256_not(val));
  TEST_ASSERT_TRUE(uint256_eq(result, val));
}

// =============================================================================
// Byte Extraction Tests
// =============================================================================

void test_uint256_byte_index_zero(void) {
  // BYTE opcode: index 0 = most significant byte
  // Value: 0x0102030405...1F20 (bytes 01-20 in big-endian)
  uint256_t val = uint256_from_limbs(0x191A1B1C1D1E1F20ULL, 0x1112131415161718ULL,
                                     0x090A0B0C0D0E0F10ULL, 0x0102030405060708ULL);
  uint256_t result = uint256_byte(uint256_from_u64(0), val);
  TEST_ASSERT_EQUAL_UINT64(0x01, result.limbs[0]);
}

void test_uint256_byte_index_31(void) {
  // BYTE opcode: index 31 = least significant byte
  uint256_t val = uint256_from_limbs(0x191A1B1C1D1E1F20ULL, 0x1112131415161718ULL,
                                     0x090A0B0C0D0E0F10ULL, 0x0102030405060708ULL);
  uint256_t result = uint256_byte(uint256_from_u64(31), val);
  TEST_ASSERT_EQUAL_UINT64(0x20, result.limbs[0]);
}

void test_uint256_byte_index_out_of_range(void) {
  uint256_t val = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);

  // Index 32 should return 0
  uint256_t result = uint256_byte(uint256_from_u64(32), val);
  TEST_ASSERT_TRUE(uint256_is_zero(result));

  // Very large index should return 0
  result = uint256_byte(uint256_from_u64(1000), val);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

// =============================================================================
// Shift Operation Tests
// =============================================================================

void test_uint256_shl_by_zero(void) {
  uint256_t val = uint256_from_u64(0x12345678);
  uint256_t result = uint256_shl(uint256_from_u64(0), val);
  TEST_ASSERT_TRUE(uint256_eq(result, val));
}

void test_uint256_shl_by_small(void) {
  // 1 << 1 = 2
  uint256_t result = uint256_shl(uint256_from_u64(1), uint256_from_u64(1));
  TEST_ASSERT_EQUAL_UINT64(2, result.limbs[0]);

  // 1 << 8 = 256
  result = uint256_shl(uint256_from_u64(8), uint256_from_u64(1));
  TEST_ASSERT_EQUAL_UINT64(256, result.limbs[0]);
}

void test_uint256_shl_cross_limb(void) {
  // Shift 1 by 64 bits - should move to limb 1
  uint256_t result = uint256_shl(uint256_from_u64(64), uint256_from_u64(1));
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[1]);

  // Shift 0xFF by 60 bits - should span limbs 0 and 1
  result = uint256_shl(uint256_from_u64(60), uint256_from_u64(0xFF));
  TEST_ASSERT_EQUAL_UINT64(0xF000000000000000ULL, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0x0F, result.limbs[1]);
}

void test_uint256_shl_by_256(void) {
  // Shift by 256 or more should return 0
  uint256_t val = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);

  uint256_t result = uint256_shl(uint256_from_u64(256), val);
  TEST_ASSERT_TRUE(uint256_is_zero(result));

  result = uint256_shl(uint256_from_u64(300), val);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_shr_by_zero(void) {
  uint256_t val = uint256_from_u64(0x12345678);
  uint256_t result = uint256_shr(uint256_from_u64(0), val);
  TEST_ASSERT_TRUE(uint256_eq(result, val));
}

void test_uint256_shr_by_small(void) {
  // 4 >> 1 = 2
  uint256_t result = uint256_shr(uint256_from_u64(1), uint256_from_u64(4));
  TEST_ASSERT_EQUAL_UINT64(2, result.limbs[0]);

  // 256 >> 8 = 1
  result = uint256_shr(uint256_from_u64(8), uint256_from_u64(256));
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[0]);
}

void test_uint256_shr_cross_limb(void) {
  // Value in limb 1, shift right by 64 should move to limb 0
  uint256_t val = uint256_from_limbs(0, 1, 0, 0);
  uint256_t result = uint256_shr(uint256_from_u64(64), val);
  TEST_ASSERT_EQUAL_UINT64(1, result.limbs[0]);
  TEST_ASSERT_EQUAL_UINT64(0, result.limbs[1]);

  // Shift that spans limbs
  val = uint256_from_limbs(0, 0xFF, 0, 0);
  result = uint256_shr(uint256_from_u64(60), val);
  TEST_ASSERT_EQUAL_UINT64(0xFF0, result.limbs[0]);
}

void test_uint256_shr_by_256(void) {
  // Shift by 256 or more should return 0
  uint256_t val = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);

  uint256_t result = uint256_shr(uint256_from_u64(256), val);
  TEST_ASSERT_TRUE(uint256_is_zero(result));

  result = uint256_shr(uint256_from_u64(300), val);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

void test_uint256_sar_positive(void) {
  // Positive value: SAR behaves like SHR
  uint256_t val = uint256_from_u64(0x100);
  uint256_t result = uint256_sar(uint256_from_u64(4), val);
  TEST_ASSERT_EQUAL_UINT64(0x10, result.limbs[0]);
}

void test_uint256_sar_negative(void) {
  // -2 (all 1s except last bit) >> 1 should give -1 (all 1s)
  uint256_t neg2 = uint256_from_limbs(UINT64_MAX - 1, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t result = uint256_sar(uint256_from_u64(1), neg2);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[0]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[1]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[2]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[3]);
}

void test_uint256_sar_negative_large_shift(void) {
  // Negative value with shift >= 256 should return -1 (all 1s)
  uint256_t neg1 = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t result = uint256_sar(uint256_from_u64(256), neg1);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[0]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[1]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[2]);
  TEST_ASSERT_EQUAL_HEX64(UINT64_MAX, result.limbs[3]);

  // Positive value with shift >= 256 should return 0
  uint256_t pos = uint256_from_u64(12345);
  result = uint256_sar(uint256_from_u64(256), pos);
  TEST_ASSERT_TRUE(uint256_is_zero(result));
}

// =============================================================================
// Signed Comparison Tests
// =============================================================================

void test_uint256_slt_both_positive(void) {
  // 5 < 10 (signed) = true
  TEST_ASSERT_TRUE(uint256_slt(uint256_from_u64(5), uint256_from_u64(10)));

  // 10 < 5 (signed) = false
  TEST_ASSERT_FALSE(uint256_slt(uint256_from_u64(10), uint256_from_u64(5)));

  // Equal values
  TEST_ASSERT_FALSE(uint256_slt(uint256_from_u64(42), uint256_from_u64(42)));
}

void test_uint256_slt_both_negative(void) {
  // -2 < -1 (signed) = true (-2 is more negative)
  uint256_t neg1 = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t neg2 = uint256_from_limbs(UINT64_MAX - 1, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  TEST_ASSERT_TRUE(uint256_slt(neg2, neg1));
  TEST_ASSERT_FALSE(uint256_slt(neg1, neg2));
}

void test_uint256_slt_mixed_signs(void) {
  uint256_t neg1 = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t pos1 = uint256_from_u64(1);

  // -1 < 1 (signed) = true
  TEST_ASSERT_TRUE(uint256_slt(neg1, pos1));

  // 1 < -1 (signed) = false
  TEST_ASSERT_FALSE(uint256_slt(pos1, neg1));
}

void test_uint256_sgt_basic(void) {
  // SGT is just SLT with arguments swapped
  uint256_t neg1 = uint256_from_limbs(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
  uint256_t pos1 = uint256_from_u64(1);

  // 1 > -1 (signed) = true
  TEST_ASSERT_TRUE(uint256_sgt(pos1, neg1));

  // -1 > 1 (signed) = false
  TEST_ASSERT_FALSE(uint256_sgt(neg1, pos1));

  // 10 > 5 (signed) = true
  TEST_ASSERT_TRUE(uint256_sgt(uint256_from_u64(10), uint256_from_u64(5)));
}