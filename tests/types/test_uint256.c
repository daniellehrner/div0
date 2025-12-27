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