#include "div0/types/uint256.h"

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

// NOLINTBEGIN(readability-magic-numbers)
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
// NOLINTEND(readability-magic-numbers)

// =============================================================================
// Division - Reciprocal-based primitives
// =============================================================================

// Reciprocal lookup table for initial approximation (~10 bits precision)
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
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
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)

// NOLINTBEGIN(readability-magic-numbers)
// 64x64 -> 128 multiplication, returning {lo, hi}
static inline void umul128(uint64_t x, uint64_t y, uint64_t *lo, uint64_t *hi) {
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
static inline void udivrem_2by1(uint64_t u_lo, uint64_t u_hi, uint64_t d, uint64_t v,
                                uint64_t *q_out, uint64_t *r_out) {
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
// NOLINTEND(readability-magic-numbers)

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
