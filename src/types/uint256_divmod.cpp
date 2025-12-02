#include <div0/types/uint256.h>

#include <array>
#include <cassert>
#include <span>
#include <tuple>
#include <utility>

namespace div0::types {

namespace {

using U128 = unsigned __int128;

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-array-to-pointer-decay,bugprone-easily-swappable-parameters)

// ============================================================================
// Reciprocal-based division primitives
// ============================================================================

/// Reciprocal lookup table for initial approximation (~10 bits precision)
// NOLINTNEXTLINE(*-avoid-c-arrays)
constexpr uint16_t RECIPROCAL_TABLE[256] = {
    // clang-format off
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
    1055, 1053, 1051, 1049, 1047, 1044, 1042, 1040, 1038, 1036, 1034, 1032, 1030, 1028, 1026, 1024
    // clang-format on
};

/// 64x64 -> 128 multiplication, returning {lo, hi}
[[gnu::always_inline]] constexpr std::pair<uint64_t, uint64_t> umul(const uint64_t x,
                                                                    const uint64_t y) noexcept {
  const U128 p = static_cast<U128>(x) * y;
  return {static_cast<uint64_t>(p), static_cast<uint64_t>(p >> 64)};
}

/// Compute reciprocal for normalized 64-bit divisor
/// Returns v such that (2^128 - 1) / d = v + 2^64
/// Requires d to be normalized (high bit set)
[[gnu::always_inline]] uint64_t reciprocal_2by1(const uint64_t d) noexcept {
  assert((d >> 63) == 1);

  const uint64_t d9 = d >> 55;
  const auto v0 = static_cast<uint64_t>(RECIPROCAL_TABLE[d9 - 256]);

  const uint64_t d40 = (d >> 24) + 1;
  const uint64_t v1 =
      (v0 << 11) - static_cast<uint32_t>(static_cast<uint32_t>(v0 * v0) * d40 >> 40) - 1;

  const uint64_t v2 = (v1 << 13) + (v1 * (0x1000000000000000ULL - v1 * d40) >> 47);

  const uint64_t d0 = d & 1;
  const uint64_t d63 = (d >> 1) + d0;
  const uint64_t e = ((v2 >> 1) & (0ULL - d0)) - (v2 * d63);
  const auto [e_lo, e_hi] = umul(v2, e);
  (void)e_lo;
  const uint64_t v3 = (e_hi >> 1) + (v2 << 31);

  const auto [p_lo, p_hi] = umul(v3, d);
  const U128 p_plus_d = static_cast<U128>(p_lo) + (static_cast<U128>(p_hi) << 64) + d;
  const uint64_t v4 = v3 - static_cast<uint64_t>(p_plus_d >> 64) - d;

  return v4;
}

/// 2-word by 1-word division with precomputed reciprocal
/// Returns {quotient, remainder}
[[gnu::always_inline]] std::pair<uint64_t, uint64_t> udivrem_2by1(const uint64_t u_lo,
                                                                  const uint64_t u_hi,
                                                                  const uint64_t d,
                                                                  const uint64_t v) noexcept {
  auto [q_lo, q_hi] = umul(v, u_hi);
  const U128 q = static_cast<U128>(q_lo) + (static_cast<U128>(q_hi) << 64) +
                 static_cast<U128>(u_lo) + (static_cast<U128>(u_hi) << 64);
  q_lo = static_cast<uint64_t>(q);
  q_hi = static_cast<uint64_t>(q >> 64);

  ++q_hi;

  auto r = u_lo - (q_hi * d);

  if (r > q_lo) {
    --q_hi;
    r += d;
  }

  if (r >= d) {
    ++q_hi;
    r -= d;
  }

  return {q_hi, r};
}

// Fast path: divide by single limb using reciprocal-based division
// Returns {quotient, remainder}
std::pair<Uint256, Uint256> divmod_single_limb(const std::span<const uint64_t, 4> dividend,
                                               const uint64_t divisor) {
  assert(divisor != 0 && "divisor must be non-zero");

  // Normalize: shift divisor left until high bit is set
  const auto shift = static_cast<unsigned>(__builtin_clzll(divisor));
  const uint64_t d_norm = divisor << shift;

  // Shift dividend left by the same amount (5 limbs to handle overflow)
  std::array<uint64_t, 5> u{};
  if (shift == 0) {
    u[0] = dividend[0];
    u[1] = dividend[1];
    u[2] = dividend[2];
    u[3] = dividend[3];
    u[4] = 0;
  } else {
    const unsigned rshift = 64 - shift;
    u[0] = dividend[0] << shift;
    u[1] = (dividend[1] << shift) | (dividend[0] >> rshift);
    u[2] = (dividend[2] << shift) | (dividend[1] >> rshift);
    u[3] = (dividend[3] << shift) | (dividend[2] >> rshift);
    u[4] = dividend[3] >> rshift;
  }

  // Compute reciprocal once for the normalized divisor
  const uint64_t reciprocal = reciprocal_2by1(d_norm);

  // Divide using reciprocal, processing from high to low
  uint64_t rem = u[4];
  std::array<uint64_t, 4> quotient{};

  std::tie(quotient[3], rem) = udivrem_2by1(u[3], rem, d_norm, reciprocal);
  std::tie(quotient[2], rem) = udivrem_2by1(u[2], rem, d_norm, reciprocal);
  std::tie(quotient[1], rem) = udivrem_2by1(u[1], rem, d_norm, reciprocal);
  std::tie(quotient[0], rem) = udivrem_2by1(u[0], rem, d_norm, reciprocal);

  // Denormalize remainder
  rem >>= shift;

  return {Uint256(quotient[0], quotient[1], quotient[2], quotient[3]), Uint256(rem)};
}

// Find index of highest non-zero limb (0-3), or -1 if zero
int top_limb_index(const std::span<const uint64_t, 4> data) {
  if (data[3] != 0) {
    return 3;
  }
  if (data[2] != 0) {
    return 2;
  }
  if (data[1] != 0) {
    return 1;
  }
  if (data[0] != 0) {
    return 0;
  }
  return -1;
}

}  // anonymous namespace

std::pair<Uint256, Uint256> Uint256::divmod(const Uint256& divisor) const noexcept {
  // D0: Handle special cases (most common first for branch prediction)

  // Division by zero - EVM returns 0
  if (divisor.is_zero()) [[unlikely]] {
    return {Uint256(0), Uint256(0)};
  }

  // Dividend < divisor - quotient is 0, remainder is dividend
  if (*this < divisor) [[unlikely]] {
    return {Uint256(0), *this};
  }

  // Fast path: both operands fit in 64 bits - use native hardware division
  // This is much faster than reciprocal-based division for single operations
  if ((data_[1] | data_[2] | data_[3]) == 0 &&
      (divisor.data_[1] | divisor.data_[2] | divisor.data_[3]) == 0) [[likely]] {
    const uint64_t q = data_[0] / divisor.data_[0];
    const uint64_t r = data_[0] % divisor.data_[0];
    return {Uint256(q), Uint256(r)};
  }

  // Find significant limbs
  const int div_top = top_limb_index(divisor.data_);

  // Single limb divisor - use reciprocal-based fast path
  if (div_top == 0) {
    return divmod_single_limb(data_, divisor.data_[0]);
  }

  // Full Knuth Algorithm D for multi-limb divisor

  // D1: Normalize - shift left so divisor's top bit is set
  // This improves quotient digit estimation accuracy
  const auto div_top_u = static_cast<size_t>(div_top);
  const int shift = __builtin_clzll(divisor.data_[div_top_u]);

  // Extended dividend needs 5 limbs to hold shifted value
  std::array<uint64_t, 5> u{};
  std::array<uint64_t, 4> v{};

  // Shift divisor
  if (shift == 0) {
    for (size_t i = 0; i <= div_top_u; ++i) {
      v[i] = divisor.data_[i];
    }
  } else {
    uint64_t carry = 0;
    for (size_t i = 0; i <= div_top_u; ++i) {
      v[i] = (divisor.data_[i] << shift) | carry;
      carry = divisor.data_[i] >> (64 - shift);
    }
  }

  // Shift dividend
  if (shift == 0) {
    for (size_t i = 0; i < 4; ++i) {
      u[i] = data_[i];
    }
  } else {
    uint64_t carry = 0;
    for (size_t i = 0; i < 4; ++i) {
      u[i] = (data_[i] << shift) | carry;
      carry = data_[i] >> (64 - shift);
    }
    u[4] = carry;
  }

  const size_t n = div_top_u + 1;  // Number of divisor limbs
  const size_t m = 4 - n;          // quotient limbs = dividend limbs - divisor limbs

  Uint256 quotient;

  // D2-D7: Main loop - compute one quotient limb per iteration
  // Process from most significant quotient digit down
  for (size_t j = m + 1; j-- > 0;) {
    // D3: Estimate quotient digit q̂ using top 2 dividend limbs / top divisor limb
    const U128 tmp = (static_cast<U128>(u[j + n]) << 64) | u[j + n - 1];
    U128 q_hat = tmp / v[n - 1];
    U128 r_hat = tmp % v[n - 1];

    // Refine estimate - q̂ may be up to 2 too large
    // This loop executes at most 2 times
    while (q_hat >= (static_cast<U128>(1) << 64) ||
           q_hat * v[n - 2] > ((r_hat << 64) | u[j + n - 2])) {
      --q_hat;
      r_hat += v[n - 1];
      if (r_hat >= (static_cast<U128>(1) << 64)) {
        break;
      }
    }

    // D4: Multiply and subtract: u[j..j+n] -= q̂ * v[0..n-1]
    // Single-pass fused multiply-subtract for better cache performance
    U128 carry = 0;
    U128 borrow = 0;
    for (size_t i = 0; i < n; ++i) {
      // Multiply: get next product limb
      const U128 prod = (q_hat * v[i]) + carry;
      carry = prod >> 64;

      // Subtract: u[j+i] -= prod_lo + borrow
      const U128 sub = (prod & ((static_cast<U128>(1) << 64) - 1)) + borrow;
      const U128 diff = static_cast<U128>(u[j + i]) + (static_cast<U128>(1) << 64) - sub;
      u[j + i] = static_cast<uint64_t>(diff);
      borrow = 1 - (diff >> 64);
    }

    // Final position: subtract carry from u[j+n]
    const U128 sub_final = carry + borrow;
    const U128 diff_final = static_cast<U128>(u[j + n]) + (static_cast<U128>(1) << 64) - sub_final;
    u[j + n] = static_cast<uint64_t>(diff_final);
    const auto final_borrow = static_cast<uint64_t>(1 - (diff_final >> 64));

    // D5: Set quotient digit
    quotient.data_[j] = static_cast<uint64_t>(q_hat);

    // D6: Add back if we subtracted too much (rare - probability < 2/base)
    if (final_borrow != 0) [[unlikely]] {
      --quotient.data_[j];
      uint64_t add_carry = 0;
      for (size_t i = 0; i < n; ++i) {
        const U128 sum = static_cast<U128>(u[j + i]) + v[i] + add_carry;
        u[j + i] = static_cast<uint64_t>(sum);
        add_carry = static_cast<uint64_t>(sum >> 64);
      }
      u[j + n] += add_carry;
    }
  }

  // D8: Denormalize remainder (shift right by the normalization amount)
  Uint256 remainder;
  if (shift == 0) {
    for (size_t i = 0; i < n; ++i) {
      remainder.data_[i] = u[i];
    }
  } else {
    for (size_t i = 0; i < n - 1; ++i) {
      remainder.data_[i] = (u[i] >> shift) | (u[i + 1] << (64 - shift));
    }
    remainder.data_[n - 1] = u[n - 1] >> shift;
  }

  return {quotient, remainder};
}

// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-array-to-pointer-decay,bugprone-easily-swappable-parameters)

}  // namespace div0::types
