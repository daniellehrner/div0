#include "div0/types/uint256.h"

#include "div0/util/hex.h"

#include <stdint.h>
#include <string.h>

// Constants for uint256 byte operations
static constexpr size_t UINT256_BYTES = 32;
static constexpr int UINT256_LIMBS = 4;
static constexpr int BYTES_PER_LIMB = 8;
static constexpr uint8_t BYTE_MASK = 0xFF;
static constexpr int BITS_PER_BYTE = 8;

// =============================================================================
// Multiplication
// =============================================================================

typedef unsigned __int128 uint128_t;

uint256_t uint256_mul(uint256_t a, uint256_t b) {
  // Schoolbook multiplication using unsigned __int128 for 64x64->128 products.
  // Only compute products where i+j < 4 (result is mod 2^256).
  //
  // Products needed:
  //   p00 = a0*b0 (col 0)
  //   p01 = a0*b1, p10 = a1*b0 (col 1)
  //   p02 = a0*b2, p11 = a1*b1, p20 = a2*b0 (col 2)
  //   a0*b3, a1*b2, a2*b1, a3*b0 (col 3, only low 64 bits needed)

  uint128_t p00 = (uint128_t)a.limbs[0] * b.limbs[0];
  uint128_t p01 = (uint128_t)a.limbs[0] * b.limbs[1];
  uint128_t p10 = (uint128_t)a.limbs[1] * b.limbs[0];
  uint128_t p02 = (uint128_t)a.limbs[0] * b.limbs[2];
  uint128_t p11 = (uint128_t)a.limbs[1] * b.limbs[1];
  uint128_t p20 = (uint128_t)a.limbs[2] * b.limbs[0];

  // Column 0: just the low 64 bits of a0*b0
  uint64_t r0 = (uint64_t)p00;

  // Column 1: high(p00) + low(p01) + low(p10)
  uint128_t col1 = (p00 >> 64) + (uint64_t)p01 + (uint64_t)p10;
  uint64_t r1 = (uint64_t)col1;

  // Column 2: carry from col1 + high(p01) + high(p10) + low(p02) + low(p11) + low(p20)
  uint128_t col2 =
      (col1 >> 64) + (p01 >> 64) + (p10 >> 64) + (uint64_t)p02 + (uint64_t)p11 + (uint64_t)p20;
  uint64_t r2 = (uint64_t)col2;

  // Column 3: carry from col2 + high(p02) + high(p11) + high(p20)
  //           + a0*b3 + a1*b2 + a2*b1 + a3*b0 (only low 64 bits needed)
  uint64_t r3 = (uint64_t)(col2 >> 64) + (uint64_t)(p02 >> 64) + (uint64_t)(p11 >> 64) +
                (uint64_t)(p20 >> 64) + (a.limbs[0] * b.limbs[3]) + (a.limbs[1] * b.limbs[2]) +
                (a.limbs[2] * b.limbs[1]) + (a.limbs[3] * b.limbs[0]);

  return uint256_from_limbs(r0, r1, r2, r3);
}

// =============================================================================
// Division - Reciprocal-based primitives
// =============================================================================

// Reciprocal lookup table for initial approximation (~10 bits precision)
static const uint16_t RECIPROCAL_TABLE[256] = {
    2045, 2037, 2029, 2021, 2013, 2005, 1998, 1990, 1983, 1975, 1968, 1960, 1953, 1946, 1938, 1931,
    1924, 1917, 1910, 1903, 1896, 1889, 1883, 1876, 1869, 1863, 1856, 1849, 1843, 1836, 1830, 1824,
    1817, 1811, 1805, 1799, 1792, 1786, 1780, 1774, 1768, 1762, 1756, 1750, 1745, 1739, 1733, 1727,
    1722, 1716, 1710, 1705, 1699, 1694, 1688, 1683, 1677, 1672, 1667, 1661, 1656, 1651, 1646, 1641,
    1636, 1630, 1625, 1620, 1615, 1610, 1605, 1600, 1596, 1591, 1586, 1581, 1576, 1572, 1567, 1562,
    1558, 1553, 1548, 1544, 1539, 1535, 1530, 1526, 1521, 1517, 1513, 1508, 1504, 1500, 1495, 1491,
    1487, 1483, 1478, 1474, 1470, 1466, 1462, 1458, 1454, 1450, 1446, 1442, 1438, 1434, 1430, 1426,
    1422, 1418, 1414, 1411, 1407, 1403, 1399, 1396, 1392, 1388, 1384, 1381, 1377, 1374, 1370, 1366,
    1363, 1359, 1356, 1352, 1349, 1345, 1342, 1338, 1335, 1332, 1328, 1325, 1322, 1318, 1315, 1312,
    1308, 1305, 1302, 1299, 1295, 1292, 1289, 1286, 1283, 1280, 1276, 1273, 1270, 1267, 1264, 1261,
    1258, 1255, 1252, 1249, 1246, 1243, 1240, 1237, 1234, 1231, 1228, 1226, 1223, 1220, 1217, 1214,
    1211, 1209, 1206, 1203, 1200, 1197, 1195, 1192, 1189, 1187, 1184, 1181, 1179, 1176, 1173, 1171,
    1168, 1165, 1163, 1160, 1158, 1155, 1153, 1150, 1148, 1145, 1143, 1140, 1138, 1135, 1133, 1130,
    1128, 1125, 1123, 1121, 1118, 1116, 1113, 1111, 1109, 1106, 1104, 1102, 1099, 1097, 1095, 1092,
    1090, 1088, 1086, 1083, 1081, 1079, 1077, 1074, 1072, 1070, 1068, 1066, 1064, 1061, 1059, 1057,
    1055, 1053, 1051, 1049, 1047, 1044, 1042, 1040, 1038, 1036, 1034, 1032, 1030, 1028, 1026, 1024};

// 64x64 -> 128 multiplication, returning {lo, hi}
static void umul128(uint64_t x, uint64_t y, uint64_t *lo, uint64_t *hi) {
  uint128_t p = (uint128_t)x * y;
  *lo = (uint64_t)p;
  *hi = (uint64_t)(p >> 64);
}

// Compute reciprocal for normalized 64-bit divisor
// Returns v such that (2^128 - 1) / d = v + 2^64
// Requires d to be normalized (high bit set)
static uint64_t reciprocal_2by1(uint64_t d) {
  uint64_t d9 = d >> 55;
  uint64_t v0 = (uint64_t)RECIPROCAL_TABLE[d9 - 256];

  uint64_t d40 = (d >> 24) + 1;
  uint64_t v1 = (v0 << 11) - (uint32_t)((uint32_t)(v0 * v0) * d40 >> 40) - 1;

  uint64_t v2 = (v1 << 13) + (v1 * (0x1000000000000000ULL - v1 * d40) >> 47);

  uint64_t d0 = d & 1;
  uint64_t d63 = (d >> 1) + d0;
  uint64_t e = ((v2 >> 1) & (0ULL - d0)) - (v2 * d63);
  uint64_t e_lo;
  uint64_t e_hi;
  umul128(v2, e, &e_lo, &e_hi);
  (void)e_lo;
  uint64_t v3 = (e_hi >> 1) + (v2 << 31);

  uint64_t p_lo;
  uint64_t p_hi;
  umul128(v3, d, &p_lo, &p_hi);
  uint128_t p_plus_d = (uint128_t)p_lo + ((uint128_t)p_hi << 64) + d;
  uint64_t v4 = v3 - (uint64_t)(p_plus_d >> 64) - d;

  return v4;
}

// 2-word by 1-word division with precomputed reciprocal
// Returns {quotient, remainder}
static void udivrem_2by1(uint64_t u_lo, uint64_t u_hi, uint64_t d, uint64_t v, uint64_t *q_out,
                         uint64_t *r_out) {
  uint64_t q_lo;
  uint64_t q_hi;
  umul128(v, u_hi, &q_lo, &q_hi);
  uint128_t q =
      (uint128_t)q_lo + ((uint128_t)q_hi << 64) + (uint128_t)u_lo + ((uint128_t)u_hi << 64);
  q_lo = (uint64_t)q;
  q_hi = (uint64_t)(q >> 64);

  ++q_hi;

  uint64_t r = u_lo - (q_hi * d);

  if (r > q_lo) {
    --q_hi;
    r += d;
  }

  if (r >= d) {
    ++q_hi;
    r -= d;
  }

  *q_out = q_hi;
  *r_out = r;
}

// Find index of highest non-zero limb (0-3), or -1 if zero
static int top_limb_index(const uint64_t *limbs) {
  if (limbs[3] != 0) {
    return 3;
  }
  if (limbs[2] != 0) {
    return 2;
  }
  if (limbs[1] != 0) {
    return 1;
  }
  if (limbs[0] != 0) {
    return 0;
  }
  return -1;
}

// Fast path: divide by single limb using reciprocal-based division
static void divmod_single_limb(const uint64_t *dividend, uint64_t divisor, uint256_t *quotient,
                               uint256_t *remainder) {
  // Normalize: shift divisor left until high bit is set
  unsigned shift = (unsigned)__builtin_clzll(divisor);
  uint64_t d_norm = divisor << shift;

  // Shift dividend left by the same amount (5 limbs to handle overflow)
  uint64_t u[5] = {0};
  if (shift == 0) {
    u[0] = dividend[0];
    u[1] = dividend[1];
    u[2] = dividend[2];
    u[3] = dividend[3];
    u[4] = 0;
  } else {
    unsigned rshift = 64 - shift;
    u[0] = dividend[0] << shift;
    u[1] = (dividend[1] << shift) | (dividend[0] >> rshift);
    u[2] = (dividend[2] << shift) | (dividend[1] >> rshift);
    u[3] = (dividend[3] << shift) | (dividend[2] >> rshift);
    u[4] = dividend[3] >> rshift;
  }

  // Compute reciprocal once for the normalized divisor
  uint64_t reciprocal = reciprocal_2by1(d_norm);

  // Divide using reciprocal, processing from high to low
  uint64_t rem = u[4];
  uint64_t q[4] = {0};

  udivrem_2by1(u[3], rem, d_norm, reciprocal, &q[3], &rem);
  udivrem_2by1(u[2], rem, d_norm, reciprocal, &q[2], &rem);
  udivrem_2by1(u[1], rem, d_norm, reciprocal, &q[1], &rem);
  udivrem_2by1(u[0], rem, d_norm, reciprocal, &q[0], &rem);

  // Denormalize remainder
  rem >>= shift;

  *quotient = uint256_from_limbs(q[0], q[1], q[2], q[3]);
  *remainder = uint256_from_u64(rem);
}

// Internal divmod function - returns both quotient and remainder
static void uint256_divmod(uint256_t dividend, uint256_t divisor, uint256_t *quotient,
                           uint256_t *remainder) {
  // D0: Handle special cases

  // Division by zero - EVM returns 0
  if (uint256_is_zero(divisor)) {
    *quotient = uint256_zero();
    *remainder = uint256_zero();
    return;
  }

  // Dividend < divisor - quotient is 0, remainder is dividend
  if (uint256_lt(dividend, divisor)) {
    *quotient = uint256_zero();
    *remainder = dividend;
    return;
  }

  // Fast path: both operands fit in 64 bits - use native hardware division
  if ((dividend.limbs[1] | dividend.limbs[2] | dividend.limbs[3]) == 0 &&
      (divisor.limbs[1] | divisor.limbs[2] | divisor.limbs[3]) == 0) {
    uint64_t q = dividend.limbs[0] / divisor.limbs[0];
    uint64_t r = dividend.limbs[0] % divisor.limbs[0];
    *quotient = uint256_from_u64(q);
    *remainder = uint256_from_u64(r);
    return;
  }

  // Find significant limbs
  int div_top = top_limb_index(divisor.limbs);

  // Single limb divisor - use reciprocal-based fast path
  if (div_top == 0) {
    divmod_single_limb(dividend.limbs, divisor.limbs[0], quotient, remainder);
    return;
  }

  // Full Knuth Algorithm D for multi-limb divisor

  // D1: Normalize - shift left so divisor's top bit is set
  size_t div_top_u = (size_t)div_top;
  int shift = __builtin_clzll(divisor.limbs[div_top_u]);

  // Extended dividend needs 5 limbs to hold shifted value
  uint64_t u[5] = {0};
  uint64_t v[4] = {0};

  // Shift divisor
  if (shift == 0) {
    for (size_t i = 0; i <= div_top_u; ++i) {
      v[i] = divisor.limbs[i];
    }
  } else {
    uint64_t carry = 0;
    for (size_t i = 0; i <= div_top_u; ++i) {
      v[i] = (divisor.limbs[i] << shift) | carry;
      carry = divisor.limbs[i] >> (64 - shift);
    }
  }

  // Shift dividend
  if (shift == 0) {
    for (size_t i = 0; i < 4; ++i) {
      u[i] = dividend.limbs[i];
    }
  } else {
    uint64_t carry = 0;
    for (size_t i = 0; i < 4; ++i) {
      u[i] = (dividend.limbs[i] << shift) | carry;
      carry = dividend.limbs[i] >> (64 - shift);
    }
    u[4] = carry;
  }

  size_t n = div_top_u + 1; // Number of divisor limbs
  size_t m = 4 - n;         // quotient limbs = dividend limbs - divisor limbs

  uint64_t q_limbs[4] = {0};

  // D2-D7: Main loop - compute one quotient limb per iteration
  for (size_t j = m + 1; j-- > 0;) {
    // D3: Estimate quotient digit q̂ using top 2 dividend limbs / top divisor
    // limb
    uint128_t tmp = ((uint128_t)u[j + n] << 64) | u[j + n - 1];
    uint128_t q_hat = tmp / v[n - 1];
    uint128_t r_hat = tmp % v[n - 1];

    // Refine estimate - q̂ may be up to 2 too large
    while (q_hat >= ((uint128_t)1 << 64) || q_hat * v[n - 2] > ((r_hat << 64) | u[j + n - 2])) {
      --q_hat;
      r_hat += v[n - 1];
      if (r_hat >= ((uint128_t)1 << 64)) {
        break;
      }
    }

    // D4: Multiply and subtract: u[j..j+n] -= q̂ * v[0..n-1]
    uint128_t carry = 0;
    uint128_t borrow = 0;
    for (size_t i = 0; i < n; ++i) {
      uint128_t prod = (q_hat * v[i]) + carry;
      carry = prod >> 64;

      uint128_t sub = (prod & (((uint128_t)1 << 64) - 1)) + borrow;
      uint128_t diff = (uint128_t)u[j + i] + ((uint128_t)1 << 64) - sub;
      u[j + i] = (uint64_t)diff;
      borrow = 1 - (diff >> 64);
    }

    // Final position: subtract carry from u[j+n]
    uint128_t sub_final = carry + borrow;
    uint128_t diff_final = (uint128_t)u[j + n] + ((uint128_t)1 << 64) - sub_final;
    u[j + n] = (uint64_t)diff_final;
    uint64_t final_borrow = (uint64_t)(1 - (diff_final >> 64));

    // D5: Set quotient digit
    q_limbs[j] = (uint64_t)q_hat;

    // D6: Add back if we subtracted too much (rare - probability < 2/base)
    if (final_borrow != 0) {
      --q_limbs[j];
      uint64_t add_carry = 0;
      for (size_t i = 0; i < n; ++i) {
        uint128_t sum = (uint128_t)u[j + i] + v[i] + add_carry;
        u[j + i] = (uint64_t)sum;
        add_carry = (uint64_t)(sum >> 64);
      }
      u[j + n] += add_carry;
    }
  }

  // D8: Denormalize remainder (shift right by the normalization amount)
  uint64_t r_limbs[4] = {0};
  if (shift == 0) {
    for (size_t i = 0; i < n; ++i) {
      r_limbs[i] = u[i];
    }
  } else {
    for (size_t i = 0; i < n - 1; ++i) {
      r_limbs[i] = (u[i] >> shift) | (u[i + 1] << (64 - shift));
    }
    r_limbs[n - 1] = u[n - 1] >> shift;
  }

  *quotient = uint256_from_limbs(q_limbs[0], q_limbs[1], q_limbs[2], q_limbs[3]);
  *remainder = uint256_from_limbs(r_limbs[0], r_limbs[1], r_limbs[2], r_limbs[3]);
}

uint256_t uint256_div(uint256_t a, uint256_t b) {
  uint256_t quotient;
  uint256_t remainder;
  uint256_divmod(a, b, &quotient, &remainder);
  return quotient;
}

uint256_t uint256_mod(uint256_t a, uint256_t b) {
  uint256_t quotient;
  uint256_t remainder;
  uint256_divmod(a, b, &quotient, &remainder);
  return remainder;
}

uint256_t uint256_from_bytes_be(const uint8_t *data, size_t len) {
  uint256_t result = uint256_zero();

  if (len == 0 || data == nullptr) {
    return result;
  }

  // Clamp length to 32 bytes
  if (len > UINT256_BYTES) {
    data += len - UINT256_BYTES;
    len = UINT256_BYTES;
  }

  // Convert big-endian bytes to little-endian limbs
  // Big-endian: data[0] is MSB, data[len-1] is LSB
  // Little-endian limbs: limbs[0] is least significant
  uint8_t padded[UINT256_BYTES] = {0};
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(padded + (UINT256_BYTES - len), data, len);

  // Now padded[0..7] goes to limbs[3] (MSB)
  // padded[24..31] goes to limbs[0] (LSB)
  for (int i = 0; i < UINT256_LIMBS; i++) {
    uint64_t limb = 0;
    size_t offset = (size_t)((UINT256_LIMBS - 1) - i) * (size_t)BYTES_PER_LIMB;
    const uint8_t *p = padded + offset;
    for (int j = 0; j < BYTES_PER_LIMB; j++) {
      limb = (limb << BITS_PER_BYTE) | p[j];
    }
    result.limbs[i] = limb;
  }

  return result;
}

void uint256_to_bytes_be(uint256_t value, uint8_t *out) {
  // Convert little-endian limbs to big-endian bytes
  // limbs[3] (MSB) goes to out[0..7]
  // limbs[0] (LSB) goes to out[24..31]
  for (int i = 0; i < UINT256_LIMBS; i++) {
    uint64_t limb = value.limbs[(UINT256_LIMBS - 1) - i];
    uint8_t *p = out + ((size_t)i * BYTES_PER_LIMB);
    for (int j = BYTES_PER_LIMB - 1; j >= 0; j--) {
      p[j] = (uint8_t)(limb & BYTE_MASK);
      limb >>= BITS_PER_BYTE;
    }
  }
}

bool uint256_from_hex(const char *hex, uint256_t *out) {
  if (out == nullptr) {
    return false;
  }
  *out = uint256_zero();

  uint8_t bytes[UINT256_BYTES];
  if (!hex_decode(hex, bytes, UINT256_BYTES)) {
    return false;
  }

  *out = uint256_from_bytes_be(bytes, UINT256_BYTES);
  return true;
}

// =============================================================================
// Helper: Two's complement negation
// =============================================================================

/// Negates a uint256 value (two's complement: -x = ~x + 1)
static inline uint256_t uint256_negate(const uint256_t a) {
  // ~a + 1
  const uint256_t not_a = uint256_from_limbs(~a.limbs[0], ~a.limbs[1], ~a.limbs[2], ~a.limbs[3]);
  return uint256_add(not_a, uint256_from_u64(1));
}

// =============================================================================
// Signed Arithmetic Operations
// =============================================================================

// MIN_VALUE = -2^255 = 0x8000...0000 (only high bit set)
static const uint256_t MIN_NEGATIVE_VALUE = {{0, 0, 0, 0x8000000000000000ULL}};

// -1 in two's complement is all 1s
static const uint256_t MINUS_ONE = {{~0ULL, ~0ULL, ~0ULL, ~0ULL}};

uint256_t uint256_sdiv(const uint256_t a, const uint256_t b) {
  // Division by zero returns 0 (EVM semantics)
  if (uint256_is_zero(b)) {
    return uint256_zero();
  }

  // Special case: MIN_VALUE / -1 would overflow (result > MAX_POSITIVE)
  // EVM returns MIN_VALUE in this case
  if (uint256_eq(a, MIN_NEGATIVE_VALUE) && uint256_eq(b, MINUS_ONE)) {
    return MIN_NEGATIVE_VALUE;
  }

  // Get signs from MSB
  const bool a_neg = uint256_is_negative(a);
  const bool b_neg = uint256_is_negative(b);

  // Convert to absolute values
  const uint256_t abs_a = a_neg ? uint256_negate(a) : a;
  const uint256_t abs_b = b_neg ? uint256_negate(b) : b;

  // Fast path: both operands fit in 64 bits - use native hardware division
  if ((abs_a.limbs[1] | abs_a.limbs[2] | abs_a.limbs[3]) == 0 &&
      (abs_b.limbs[1] | abs_b.limbs[2] | abs_b.limbs[3]) == 0) {
    const uint64_t q = abs_a.limbs[0] / abs_b.limbs[0];
    // Apply sign: negative if exactly one operand was negative
    if (a_neg != b_neg) {
      return uint256_negate(uint256_from_u64(q));
    }
    return uint256_from_u64(q);
  }

  // Perform unsigned division for multi-limb case
  const uint256_t quotient = uint256_div(abs_a, abs_b);

  // Apply sign: negative if exactly one operand was negative
  return (a_neg != b_neg) ? uint256_negate(quotient) : quotient;
}

uint256_t uint256_smod(const uint256_t a, const uint256_t b) {
  // Modulo by zero returns 0 (EVM semantics)
  if (uint256_is_zero(b)) {
    return uint256_zero();
  }

  // Get signs
  const bool a_neg = uint256_is_negative(a);
  const bool b_neg = uint256_is_negative(b);

  // Convert to absolute values
  const uint256_t abs_a = a_neg ? uint256_negate(a) : a;
  const uint256_t abs_b = b_neg ? uint256_negate(b) : b;

  // Fast path: both operands fit in 64 bits - use native hardware modulo
  if ((abs_a.limbs[1] | abs_a.limbs[2] | abs_a.limbs[3]) == 0 &&
      (abs_b.limbs[1] | abs_b.limbs[2] | abs_b.limbs[3]) == 0) {
    const uint64_t r = abs_a.limbs[0] % abs_b.limbs[0];
    // Result sign follows dividend sign (EVM semantics)
    if (a_neg) {
      return uint256_negate(uint256_from_u64(r));
    }
    return uint256_from_u64(r);
  }

  // Perform unsigned modulo for multi-limb case
  const uint256_t remainder = uint256_mod(abs_a, abs_b);

  // Result sign follows dividend sign (EVM semantics)
  return a_neg ? uint256_negate(remainder) : remainder;
}

uint256_t uint256_signextend(const uint256_t byte_pos, const uint256_t x) {
  // If byte_pos >= 31 (or any upper limbs set), the value already uses all 256 bits
  // No sign extension needed
  if (byte_pos.limbs[1] != 0 || byte_pos.limbs[2] != 0 || byte_pos.limbs[3] != 0 ||
      byte_pos.limbs[0] >= 31) {
    return x;
  }

  // byte_pos is 0-30, indicating which byte's MSB is the sign bit
  // Byte 0 = bits 0-7, Byte 1 = bits 8-15, etc.
  // Sign bit is at position: (8 * byte_pos) + 7
  const uint64_t pos = byte_pos.limbs[0];
  const unsigned bit_index = (unsigned)((8 * pos) + 7);

  // Determine which limb contains the sign bit
  const unsigned limb_index = bit_index / 64;
  const unsigned bit_in_limb = bit_index % 64;

  // Check if the sign bit is set
  const bool sign_bit = ((x.limbs[limb_index] >> bit_in_limb) & 1) != 0;

  uint256_t result = x;

  if (!sign_bit) {
    // Positive: clear all bits above the sign bit position
    // Create a mask with 1s from bit 0 to bit_index (inclusive)

    // Clear bits above sign bit in the containing limb
    // Handle bit_in_limb == 63 specially to avoid shift overflow
    const uint64_t limb_mask = (bit_in_limb == 63) ? ~0ULL : (1ULL << (bit_in_limb + 1)) - 1;
    result.limbs[limb_index] &= limb_mask;

    // Clear all higher limbs
    for (unsigned i = limb_index + 1; i < 4; ++i) {
      result.limbs[i] = 0;
    }
  } else {
    // Negative: set all bits above the sign bit position to 1

    // Set bits above sign bit in the containing limb to 1
    // Handle bit_in_limb == 63 specially to avoid shift overflow
    const uint64_t limb_mask = (bit_in_limb == 63) ? 0ULL : ~((1ULL << (bit_in_limb + 1)) - 1);
    result.limbs[limb_index] |= limb_mask;

    // Set all higher limbs to all 1s
    for (unsigned i = limb_index + 1; i < 4; ++i) {
      result.limbs[i] = ~0ULL;
    }
  }

  return result;
}

// =============================================================================
// Modular Arithmetic Operations
// =============================================================================

uint256_t uint256_addmod(const uint256_t a, const uint256_t b, const uint256_t n) {
  // Return 0 if modulus is zero (EVM semantics)
  if (uint256_is_zero(n)) {
    return uint256_zero();
  }

  // Compute a + b with potential 257-bit result
  // We need to handle carry properly
  uint256_t sum;
  unsigned long long carry = 0;
  sum.limbs[0] = __builtin_addcll(a.limbs[0], b.limbs[0], carry, &carry);
  sum.limbs[1] = __builtin_addcll(a.limbs[1], b.limbs[1], carry, &carry);
  sum.limbs[2] = __builtin_addcll(a.limbs[2], b.limbs[2], carry, &carry);
  sum.limbs[3] = __builtin_addcll(a.limbs[3], b.limbs[3], carry, &carry);

  if (carry == 0) {
    // No overflow - simple modulo
    return uint256_mod(sum, n);
  }

  // We have a 257-bit number: carry bit + sum
  // Need to compute (carry * 2^256 + sum) mod n
  // = ((carry * 2^256) mod n + sum mod n) mod n

  // First, compute 2^256 mod n
  // 2^256 = (2^256 - n) + n, so 2^256 mod n = (2^256 - n) mod n if 2^256 > n
  // Since n <= 2^256 - 1, we have 2^256 - n >= 1
  // We compute this as: (MAX_UINT256 + 1 - n) mod n = (MAX_UINT256 - n + 1) mod n
  // But MAX_UINT256 + 1 = 0 (overflow), so we compute differently:
  // 2^256 mod n = ((2^256 - 1) - (n - 1)) mod n = (MAX - (n-1)) mod n = (MAX - n + 1) mod n
  // Since MAX = 2^256 - 1, MAX - n + 1 = 2^256 - n which is < 2^256

  const uint256_t max_val = uint256_from_limbs(~0ULL, ~0ULL, ~0ULL, ~0ULL);
  // pow_256_mod_n = (MAX - n + 1) mod n = (MAX - (n - 1)) mod n
  // = uint256_sub(max_val, uint256_sub(n, one)) mod n
  // Simplify: if n <= MAX, then MAX - n + 1 < 2^256, so we can compute directly
  const uint256_t pow_256_mod_n =
      uint256_mod(uint256_add(uint256_sub(max_val, n), uint256_from_u64(1)), n);

  // Result = (carry * pow_256_mod_n + sum) mod n
  // Since carry is 1, this is just (pow_256_mod_n + sum mod n) mod n
  const uint256_t sum_mod_n = uint256_mod(sum, n);
  const uint256_t partial = uint256_add(pow_256_mod_n, sum_mod_n);

  // Check if this addition overflowed
  if (uint256_lt(partial, pow_256_mod_n)) {
    // Overflow - need to handle 257-bit case again (recursive case very rare)
    // Actually, since pow_256_mod_n < n and sum_mod_n < n, their sum < 2n < 2^256
    // So we just need to subtract n if result >= n
    return uint256_mod(partial, n);
  }

  // No overflow, but result might be >= n
  if (!uint256_lt(partial, n)) {
    return uint256_sub(partial, n);
  }
  return partial;
}

/// 512-bit intermediate result for mulmod
typedef struct {
  uint64_t limbs[8];
} uint512_t;

/// Multiply two uint256 values to get full 512-bit result
static uint512_t uint256_mul_full(const uint256_t a, const uint256_t b) {
  uint512_t result = {{0}};

  // Schoolbook multiplication for full 512-bit result
  for (int i = 0; i < 4; i++) {
    uint128_t carry = 0;
    for (int j = 0; j < 4; j++) {
      const uint128_t prod = ((uint128_t)a.limbs[i] * b.limbs[j]) + result.limbs[i + j] + carry;
      result.limbs[i + j] = (uint64_t)prod;
      carry = prod >> 64;
    }
    result.limbs[i + 4] = (uint64_t)carry;
  }

  return result;
}

/// Check if 512-bit value is zero
static bool uint512_is_zero(const uint512_t a) {
  return (a.limbs[0] | a.limbs[1] | a.limbs[2] | a.limbs[3] | a.limbs[4] | a.limbs[5] | a.limbs[6] |
          a.limbs[7]) == 0;
}

/// 512-bit mod 256-bit using schoolbook division
static uint256_t uint512_mod_256(const uint512_t a, const uint256_t b) {
  // Handle special cases
  if (uint256_is_zero(b)) {
    return uint256_zero();
  }

  if (uint512_is_zero(a)) {
    return uint256_zero();
  }

  // If a fits in 256 bits, use regular mod
  if ((a.limbs[4] | a.limbs[5] | a.limbs[6] | a.limbs[7]) == 0) {
    const uint256_t a_256 = uint256_from_limbs(a.limbs[0], a.limbs[1], a.limbs[2], a.limbs[3]);
    return uint256_mod(a_256, b);
  }

  // Find highest non-zero limb in divisor
  int div_top = -1;
  for (int i = 3; i >= 0; i--) {
    if (b.limbs[i] != 0) {
      div_top = i;
      break;
    }
  }

  // Normalize divisor (shift so MSB is set)
  const int shift = __builtin_clzll(b.limbs[div_top]);

  // Shift divisor
  uint256_t v = uint256_zero();
  if (shift == 0) {
    v = b;
  } else {
    uint64_t carry = 0;
    for (int i = 0; i <= div_top; i++) {
      v.limbs[i] = (b.limbs[i] << shift) | carry;
      carry = b.limbs[i] >> (64 - shift);
    }
  }

  // Shift dividend (need 9 limbs to handle overflow)
  uint64_t u[9] = {0};
  if (shift == 0) {
    for (int i = 0; i < 8; i++) {
      u[i] = a.limbs[i];
    }
  } else {
    uint64_t carry = 0;
    for (int i = 0; i < 8; i++) {
      u[i] = (a.limbs[i] << shift) | carry;
      carry = a.limbs[i] >> (64 - shift);
    }
    u[8] = carry;
  }

  const size_t n = (size_t)div_top + 1; // Number of divisor limbs
  const size_t m = 8 - n;               // Number of quotient limbs

  // Knuth's Algorithm D for multi-precision division
  for (size_t j = m + 1; j-- > 0;) {
    // Estimate quotient digit
    const uint128_t tmp = ((uint128_t)u[j + n] << 64) | u[j + n - 1];
    uint128_t q_hat = tmp / v.limbs[n - 1];
    uint128_t r_hat = tmp % v.limbs[n - 1];

    // Refine estimate
    while (q_hat >= ((uint128_t)1 << 64) ||
           (n >= 2 && q_hat * v.limbs[n - 2] > ((r_hat << 64) | u[j + n - 2]))) {
      --q_hat;
      r_hat += v.limbs[n - 1];
      if (r_hat >= ((uint128_t)1 << 64)) {
        break;
      }
    }

    // Multiply and subtract
    uint128_t carry = 0;
    uint128_t borrow = 0;
    for (size_t i = 0; i < n; ++i) {
      const uint128_t prod = (q_hat * v.limbs[i]) + carry;
      carry = prod >> 64;
      const uint128_t sub_val = (prod & (((uint128_t)1 << 64) - 1)) + borrow;
      const uint128_t diff = (uint128_t)u[j + i] + ((uint128_t)1 << 64) - sub_val;
      u[j + i] = (uint64_t)diff;
      borrow = 1 - (diff >> 64);
    }

    const uint128_t sub_final = carry + borrow;
    const uint128_t diff_final = (uint128_t)u[j + n] + ((uint128_t)1 << 64) - sub_final;
    u[j + n] = (uint64_t)diff_final;
    const uint64_t final_borrow = (uint64_t)(1 - (diff_final >> 64));

    // Add back if needed
    if (final_borrow != 0) {
      uint64_t add_carry = 0;
      for (size_t i = 0; i < n; ++i) {
        const uint128_t sum = (uint128_t)u[j + i] + v.limbs[i] + add_carry;
        u[j + i] = (uint64_t)sum;
        add_carry = (uint64_t)(sum >> 64);
      }
      u[j + n] += add_carry;
    }
  }

  // Denormalize remainder
  uint64_t r_limbs[4] = {0};
  if (shift == 0) {
    for (size_t i = 0; i < n; ++i) {
      r_limbs[i] = u[i];
    }
  } else {
    for (size_t i = 0; i < n - 1; ++i) {
      r_limbs[i] = (u[i] >> shift) | (u[i + 1] << (64 - shift));
    }
    r_limbs[n - 1] = u[n - 1] >> shift;
  }

  return uint256_from_limbs(r_limbs[0], r_limbs[1], r_limbs[2], r_limbs[3]);
}

uint256_t uint256_mulmod(const uint256_t a, const uint256_t b, const uint256_t n) {
  // Return 0 if modulus is zero (EVM semantics)
  if (uint256_is_zero(n)) {
    return uint256_zero();
  }

  // Return 0 if either operand is zero
  if (uint256_is_zero(a) || uint256_is_zero(b)) {
    return uint256_zero();
  }

  // Compute full 512-bit product
  const uint512_t product = uint256_mul_full(a, b);

  // Fast path: if product fits in 256 bits, use regular mod
  if ((product.limbs[4] | product.limbs[5] | product.limbs[6] | product.limbs[7]) == 0) {
    const uint256_t prod_256 =
        uint256_from_limbs(product.limbs[0], product.limbs[1], product.limbs[2], product.limbs[3]);
    return uint256_mod(prod_256, n);
  }

  // General case: 512-bit mod 256-bit
  return uint512_mod_256(product, n);
}

// =============================================================================
// Exponentiation
// =============================================================================

uint256_t uint256_exp(const uint256_t base, const uint256_t exponent) {
  // Special cases for quick exit
  if (uint256_is_zero(exponent)) {
    return uint256_from_u64(1); // x^0 = 1
  }

  if (uint256_is_zero(base)) {
    return uint256_zero(); // 0^n = 0 (for n > 0)
  }

  if (uint256_eq(base, uint256_from_u64(1))) {
    return uint256_from_u64(1); // 1^n = 1
  }

  // Optimization: if base is even and exponent is large, result will overflow to 0
  // An even base raised to power >= 256 will be 0 mod 2^256
  // (2^k)^n = 2^(k*n), and k*n >= 256 when n >= 256/k
  // For base divisible by 2, any exponent > 255 gives 0
  if ((base.limbs[0] & 1) == 0 && (exponent.limbs[0] > 255 || exponent.limbs[1] != 0 ||
                                   exponent.limbs[2] != 0 || exponent.limbs[3] != 0)) {
    return uint256_zero();
  }

  // Binary exponentiation (square-and-multiply)
  uint256_t result = uint256_from_u64(1);
  uint256_t multiplier = base;
  uint256_t exp = exponent;

  while (!uint256_is_zero(exp)) {
    if ((exp.limbs[0] & 1) != 0) {
      result = uint256_mul(result, multiplier);
    }
    multiplier = uint256_mul(multiplier, multiplier);
    // Right shift exp by 1
    exp.limbs[0] = (exp.limbs[0] >> 1) | (exp.limbs[1] << 63);
    exp.limbs[1] = (exp.limbs[1] >> 1) | (exp.limbs[2] << 63);
    exp.limbs[2] = (exp.limbs[2] >> 1) | (exp.limbs[3] << 63);
    exp.limbs[3] = exp.limbs[3] >> 1;
  }

  return result;
}

size_t uint256_byte_length(const uint256_t value) {
  // Find highest non-zero limb
  for (int i = 3; i >= 0; i--) {
    if (value.limbs[i] != 0) {
      // Find highest non-zero byte in this limb
      const uint64_t limb = value.limbs[i];
      int byte_pos = 7;
      while (byte_pos >= 0 && ((limb >> (byte_pos * 8)) & 0xFF) == 0) {
        byte_pos--;
      }
      return ((size_t)i * 8) + (size_t)byte_pos + 1;
    }
  }
  return 0; // Value is zero
}
