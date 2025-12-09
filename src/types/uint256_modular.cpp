// Modular arithmetic for Uint256 (addmod, mulmod)

#include <div0/types/uint256.h>

#include <array>
#include <cassert>
#include <tuple>

namespace div0::types {

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic,bugprone-easily-swappable-parameters)

namespace {

using U128 = unsigned __int128;

// ============================================================================
// Low-level primitives
// ============================================================================

/// Result of operation with carry/borrow flag
struct ResultWithCarry {
  uint64_t value;
  bool carry;
};

/// Addition with carry
[[gnu::always_inline]] constexpr ResultWithCarry addc(const uint64_t x, const uint64_t y,
                                                      const bool carry_in = false) noexcept {
  unsigned long long carry_out = 0;
  const auto sum = __builtin_addcll(x, y, static_cast<unsigned long long>(carry_in), &carry_out);
  return {.value = sum, .carry = carry_out != 0};
}

/// Subtraction with borrow
[[gnu::always_inline]] constexpr ResultWithCarry subc(const uint64_t x, const uint64_t y,
                                                      const bool borrow_in = false) noexcept {
  unsigned long long borrow_out = 0;
  const auto diff = __builtin_subcll(x, y, static_cast<unsigned long long>(borrow_in), &borrow_out);
  return {.value = diff, .carry = borrow_out != 0};
}

/// 64x64 -> 128 multiplication, returning {lo, hi}
[[gnu::always_inline]] constexpr std::pair<uint64_t, uint64_t> umul(const uint64_t x,
                                                                    const uint64_t y) noexcept {
  const U128 p = static_cast<U128>(x) * y;
  return {static_cast<uint64_t>(p), static_cast<uint64_t>(p >> 64)};
}

/// Fast 128-bit addition using compiler's __int128
[[gnu::always_inline]] constexpr U128 fast_add_128(const U128 x, const U128 y) noexcept {
  return x + y;
}

/// Count leading zeros for 64-bit value
[[gnu::always_inline]] constexpr unsigned clz64(const uint64_t x) noexcept {
  return static_cast<unsigned>(__builtin_clzll(x));
}

// ============================================================================
// Reciprocal computation
// ============================================================================

/// Reciprocal lookup table for initial approximation (~10 bits precision)
/// Entry i corresponds to divisor with top 9 bits = i + 256
constexpr std::array<uint16_t, 256> RECIPROCAL_TABLE = {
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

/// Compute reciprocal for normalized 64-bit divisor
/// Returns v such that (2^128 - 1) / d = v + 2^64
/// Requires d to be normalized (high bit set)
[[gnu::always_inline]] uint64_t reciprocal_2by1(const uint64_t d) noexcept {
  assert((d >> 63) == 1);

  const uint64_t d9 = d >> 55;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
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

/// Compute reciprocal for normalized 128-bit divisor {d_lo, d_hi}
[[gnu::always_inline]] uint64_t reciprocal_3by2(const uint64_t d_lo, const uint64_t d_hi) noexcept {
  auto v = reciprocal_2by1(d_hi);
  auto p = d_hi * v;
  p += d_lo;

  if (p < d_lo) {
    --v;
    if (p >= d_hi) {
      --v;
      p -= d_hi;
    }
    p -= d_hi;
  }

  const auto [t_lo, t_hi] = umul(v, d_lo);

  p += t_hi;
  if (p < t_hi) {
    --v;
    if (p >= d_hi) {
      if (p > d_hi || t_lo >= d_lo) {
        --v;
      }
    }
  }

  return v;
}

// ============================================================================
// Division primitives
// ============================================================================

/// 2-word by 1-word division with precomputed reciprocal
/// Returns {quotient, remainder}
[[gnu::always_inline]] std::pair<uint64_t, uint64_t> udivrem_2by1(const uint64_t u_lo,
                                                                  const uint64_t u_hi,
                                                                  const uint64_t d,
                                                                  const uint64_t v) noexcept {
  auto [q_lo, q_hi] = umul(v, u_hi);
  const U128 q = fast_add_128(static_cast<U128>(q_lo) | (static_cast<U128>(q_hi) << 64),
                              static_cast<U128>(u_lo) | (static_cast<U128>(u_hi) << 64));
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

/// 3-word by 2-word division with precomputed reciprocal
/// Returns {quotient, remainder_lo, remainder_hi}
[[gnu::always_inline]] std::tuple<uint64_t, uint64_t, uint64_t> udivrem_3by2(
    const uint64_t u2, const uint64_t u1, const uint64_t u0, const uint64_t d_lo,
    const uint64_t d_hi, const uint64_t v) noexcept {
  auto [q_lo, q_hi] = umul(v, u2);
  const U128 q = fast_add_128(static_cast<U128>(q_lo) | (static_cast<U128>(q_hi) << 64),
                              static_cast<U128>(u1) | (static_cast<U128>(u2) << 64));
  q_lo = static_cast<uint64_t>(q);
  q_hi = static_cast<uint64_t>(q >> 64);

  auto r1 = u1 - (q_hi * d_hi);

  const auto [t_lo, t_hi] = umul(d_lo, q_hi);

  const U128 t = static_cast<U128>(t_lo) | (static_cast<U128>(t_hi) << 64);
  const U128 d = static_cast<U128>(d_lo) | (static_cast<U128>(d_hi) << 64);
  U128 r = (static_cast<U128>(u0) | (static_cast<U128>(r1) << 64)) - t - d;
  r1 = static_cast<uint64_t>(r >> 64);

  ++q_hi;

  if (r1 >= q_lo) {
    --q_hi;
    r += d;
  }

  if (r >= d) {
    ++q_hi;
    r -= d;
  }

  return {q_hi, static_cast<uint64_t>(r), static_cast<uint64_t>(r >> 64)};
}

/// Divide multi-word number by 1-word divisor using reciprocal
/// u[] is modified in-place to contain quotient, returns remainder
uint64_t udivrem_by1(uint64_t* u, const int len, const uint64_t d) noexcept {
  assert(len >= 2);

  const auto reciprocal = reciprocal_2by1(d);

  auto rem = u[len - 1];
  u[len - 1] = 0;

  for (int i = len - 2; i >= 0; --i) {
    std::tie(u[i], rem) = udivrem_2by1(u[i], rem, d, reciprocal);
  }

  return rem;
}

/// Divide multi-word number by 2-word divisor using reciprocal
/// u[] is modified in-place to contain quotient, returns {rem_lo, rem_hi}
std::pair<uint64_t, uint64_t> udivrem_by2(uint64_t* u, int len, const uint64_t d_lo,
                                          const uint64_t d_hi) noexcept {
  assert(len >= 3);

  const auto reciprocal = reciprocal_3by2(d_lo, d_hi);

  uint64_t rem_lo = u[len - 2];
  uint64_t rem_hi = u[len - 1];
  u[len - 1] = u[len - 2] = 0;

  for (int i = len - 3; i >= 0; --i) {
    std::tie(u[i], rem_lo, rem_hi) = udivrem_3by2(rem_hi, rem_lo, u[i], d_lo, d_hi, reciprocal);
  }

  return {rem_lo, rem_hi};
}

/// s = x + y, returns carry
bool add_words(uint64_t* s, const uint64_t* x, const uint64_t* y, const int len) noexcept {
  assert(len >= 2);

  bool carry = false;
  for (int i = 0; i < len; ++i) {
    auto [sum, c] = addc(x[i], y[i], carry);
    s[i] = sum;
    carry = c;
  }
  return carry;
}

/// r = x - multiplier * y, returns borrow
uint64_t submul(uint64_t* r, const uint64_t* x, const uint64_t* y, const int len,
                const uint64_t multiplier) noexcept {
  assert(len >= 1);

  uint64_t borrow = 0;
  for (int i = 0; i < len; ++i) {
    const auto s = x[i] - borrow;
    const auto [p_lo, p_hi] = umul(y[i], multiplier);
    borrow = p_hi + static_cast<uint64_t>(x[i] < s);
    r[i] = s - p_lo;
    borrow += static_cast<uint64_t>(s < r[i]);
  }

  return borrow;
}

/// Knuth's Algorithm D for 3+ word divisors
void udivrem_knuth(uint64_t* q, uint64_t* u, const int u_len, const uint64_t* d,
                   const int d_len) noexcept {
  assert(d_len >= 3);
  assert(u_len >= d_len);

  const uint64_t d_hi = d[d_len - 1];
  const uint64_t d_lo = d[d_len - 2];
  const auto reciprocal = reciprocal_3by2(d_lo, d_hi);

  for (int j = u_len - d_len - 1; j >= 0; --j) {
    const auto u2 = u[j + d_len];
    const auto u1 = u[j + d_len - 1];
    const auto u0 = u[j + d_len - 2];

    uint64_t qhat{0};
    const U128 u_top = static_cast<U128>(u1) | (static_cast<U128>(u2) << 64);
    const U128 d_top = static_cast<U128>(d_lo) | (static_cast<U128>(d_hi) << 64);

    if (u_top == d_top) [[unlikely]] {
      // Division overflows - quotient is MAX64
      qhat = ~uint64_t{0};
      u[j + d_len] = u2 - submul(&u[j], &u[j], d, d_len, qhat);
    } else {
      uint64_t rhat_lo{0};
      uint64_t rhat_hi{0};
      std::tie(qhat, rhat_lo, rhat_hi) = udivrem_3by2(u2, u1, u0, d_lo, d_hi, reciprocal);

      // Subtract qhat * d[0..d_len-3] from u[j..j+d_len-3]
      const auto overflow = submul(&u[j], &u[j], d, d_len - 2, qhat);

      auto [new_rhat_lo, carry1] = subc(rhat_lo, overflow);
      auto [new_rhat_hi, carry2] = subc(rhat_hi, static_cast<uint64_t>(carry1));

      if (carry2) [[unlikely]] {
        --qhat;
        new_rhat_hi += d_hi + static_cast<uint64_t>(add_words(&u[j], &u[j], d, d_len - 1));
      }

      u[j + d_len - 2] = new_rhat_lo;
      u[j + d_len - 1] = new_rhat_hi;
    }

    q[j] = qhat;
  }
}

// ============================================================================
// Normalization helpers
// ============================================================================

/// Count significant words in array (words with non-zero value from high to low)
int count_significant_words(const uint64_t* data, const int max_words) noexcept {
  for (int i = max_words; i > 0; --i) {
    if (data[i - 1] != 0) {
      return i;
    }
  }
  return 0;
}

/// Normalize dividend and divisor by shifting left until divisor's high bit is set
/// Returns shift amount
unsigned normalize_for_division(uint64_t* un, uint64_t* vn, const int vn_len, const uint64_t* u,
                                const int u_len, const uint64_t* v, const int v_len) noexcept {
  const unsigned shift = clz64(v[v_len - 1]);

  if (shift != 0) {
    // Normalize divisor
    for (int i = vn_len - 1; i > 0; --i) {
      vn[i] = (v[i] << shift) | (v[i - 1] >> (64 - shift));
    }
    vn[0] = v[0] << shift;

    // Normalize numerator
    un[u_len] = u[u_len - 1] >> (64 - shift);
    for (int i = u_len - 1; i > 0; --i) {
      un[i] = (u[i] << shift) | (u[i - 1] >> (64 - shift));
    }
    un[0] = u[0] << shift;
  } else {
    for (int i = 0; i < v_len; ++i) {
      vn[i] = v[i];
    }
    for (int i = 0; i < u_len; ++i) {
      un[i] = u[i];
    }
    un[u_len] = 0;
  }

  return shift;
}

/// Denormalize remainder by shifting right
void denormalize_remainder(uint64_t* r, const uint64_t* un, const int r_len,
                           const unsigned shift) noexcept {
  if (shift == 0) {
    for (int i = 0; i < r_len; ++i) {
      r[i] = un[i];
    }
  } else {
    for (int i = 0; i < r_len - 1; ++i) {
      r[i] = (un[i] >> shift) | (un[i + 1] << (64 - shift));
    }
    r[r_len - 1] = un[r_len - 1] >> shift;
  }
}

// ============================================================================
// Main division: 512-bit by 256-bit -> 256-bit quotient and remainder
// ============================================================================

/// Divide 512-bit number by 256-bit divisor
/// Returns remainder as Uint256
Uint256 udivrem_512_256(const uint64_t* u, int u_len, const uint64_t* v, int v_len) noexcept {
  // Trim leading zeros
  while (u_len > 0 && u[u_len - 1] == 0) {
    --u_len;
  }
  if (u_len == 0) {
    return {0};
  }

  // Handle u < v case
  if (u_len < v_len) {
    return Uint256(u[0], u_len > 1 ? u[1] : 0, u_len > 2 ? u[2] : 0, u_len > 3 ? u[3] : 0);
  }

  if (u_len == v_len) {
    // Compare from high to low
    bool less = false;
    for (int i = u_len - 1; i >= 0; --i) {
      if (u[i] < v[i]) {
        less = true;
        break;
      }
      if (u[i] > v[i]) {
        break;
      }
    }
    if (less) {
      return Uint256(u[0], u_len > 1 ? u[1] : 0, u_len > 2 ? u[2] : 0, u_len > 3 ? u[3] : 0);
    }
  }

  // Normalize
  std::array<uint64_t, 10> un = {};  // Max 9 words + 1 for shift overflow
  std::array<uint64_t, 4> vn = {};

  const unsigned shift = normalize_for_division(un.data(), vn.data(), 4, u, u_len, v, v_len);

  auto un_len = static_cast<size_t>(u_len);
  if (un[static_cast<size_t>(u_len)] != 0 ||
      un[static_cast<size_t>(u_len - 1)] >= vn[static_cast<size_t>(v_len - 1)]) {
    ++un_len;
  }

  // Fast path: single-word divisor
  if (v_len == 1) {
    const uint64_t rem = udivrem_by1(un.data(), static_cast<int>(un_len), vn[0]);
    return {rem >> shift};
  }

  // Fast path: two-word divisor
  if (v_len == 2) {
    const auto [rem_lo, rem_hi] = udivrem_by2(un.data(), static_cast<int>(un_len), vn[0], vn[1]);
    // Denormalize remainder
    const uint64_t r_lo = shift == 0 ? rem_lo : (rem_lo >> shift) | (rem_hi << (64 - shift));
    const uint64_t r_hi = rem_hi >> shift;
    return Uint256(r_lo, r_hi, 0, 0);
  }

  // General case: 3+ word divisor
  std::array<uint64_t, 6> q = {};
  udivrem_knuth(q.data(), un.data(), static_cast<int>(un_len), vn.data(), v_len);

  // Denormalize remainder
  std::array<uint64_t, 4> r = {};
  denormalize_remainder(r.data(), un.data(), v_len, shift);

  return Uint256(r[0], v_len > 1 ? r[1] : 0, v_len > 2 ? r[2] : 0, v_len > 3 ? r[3] : 0);
}

/// Compute full 512-bit product of two 256-bit values
void umul_512(const uint64_t* x, const uint64_t* y, uint64_t* result) noexcept {
  // Zero initialize result
  for (int i = 0; i < 8; ++i) {
    result[i] = 0;
  }

  // Schoolbook multiplication with carry propagation
  for (int j = 0; j < 4; ++j) {
    uint64_t k = 0;
    for (int i = 0; i < 4; ++i) {
      auto [a_val, a_carry] = addc(result[i + j], k);
      auto [p_lo, p_hi] = umul(x[i], y[j]);
      const U128 t = static_cast<U128>(p_lo) + (static_cast<U128>(p_hi) << 64) +
                     static_cast<U128>(a_val) + (static_cast<U128>(a_carry) << 64);
      result[i + j] = static_cast<uint64_t>(t);
      k = static_cast<uint64_t>(t >> 64);
    }
    result[j + 4] = k;
  }
}

}  // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Uint256 Uint256::addmod(const Uint256& a, const Uint256& b, const Uint256& modulus) noexcept {
  if (modulus.is_zero()) [[unlikely]] {
    return {0};
  }

  const auto& m = modulus.data_;

  // Fast path for mod >= 2^192, with x and y at most slightly bigger than mod
  if ((m[3] != 0) && (a.data_[3] <= m[3]) && (b.data_[3] <= m[3])) {
    // Normalize a in case it is >= mod
    auto [ax0, bx0] = subc(a.data_[0], m[0]);
    auto [ax1, bx1] = subc(a.data_[1], m[1], bx0);
    auto [ax2, bx2] = subc(a.data_[2], m[2], bx1);
    auto [ax3, bx3] = subc(a.data_[3], m[3], bx2);

    const uint64_t xn0 = bx3 ? a.data_[0] : ax0;
    const uint64_t xn1 = bx3 ? a.data_[1] : ax1;
    const uint64_t xn2 = bx3 ? a.data_[2] : ax2;
    const uint64_t xn3 = bx3 ? a.data_[3] : ax3;

    // Normalize b in case it is >= mod
    auto [ay0, by0] = subc(b.data_[0], m[0]);
    auto [ay1, by1] = subc(b.data_[1], m[1], by0);
    auto [ay2, by2] = subc(b.data_[2], m[2], by1);
    auto [ay3, by3] = subc(b.data_[3], m[3], by2);

    const uint64_t yn0 = by3 ? b.data_[0] : ay0;
    const uint64_t yn1 = by3 ? b.data_[1] : ay1;
    const uint64_t yn2 = by3 ? b.data_[2] : ay2;
    const uint64_t yn3 = by3 ? b.data_[3] : ay3;

    // Add normalized values
    auto [s0, c0] = addc(xn0, yn0);
    auto [s1, c1] = addc(xn1, yn1, c0);
    auto [s2, c2] = addc(xn2, yn2, c1);
    auto [s3, c3] = addc(xn3, yn3, c2);

    // Subtract mod from sum
    auto [r0, b0] = subc(s0, m[0]);
    auto [r1, b1] = subc(s1, m[1], b0);
    auto [r2, b2] = subc(s2, m[2], b1);
    auto [r3, b3] = subc(s3, m[3], b2);

    // If sum overflowed OR subtraction didn't borrow, use reduced result
    if (c3 || !b3) {
      return Uint256(r0, r1, r2, r3);
    }
    return Uint256(s0, s1, s2, s3);
  }

  // General path: compute 320-bit sum
  auto [s0, c0] = addc(a.data_[0], b.data_[0]);
  auto [s1, c1] = addc(a.data_[1], b.data_[1], c0);
  auto [s2, c2] = addc(a.data_[2], b.data_[2], c1);
  auto [s3, c3] = addc(a.data_[3], b.data_[3], c2);

  if (!c3) {
    const Uint256 sum(s0, s1, s2, s3);
    if (sum < modulus) {
      return sum;
    }
    return sum % modulus;
  }

  // 320-bit sum: divide by mod
  std::array<uint64_t, 5> sum = {s0, s1, s2, s3, 1};
  const int v_len = count_significant_words(m.data(), 4);
  return udivrem_512_256(sum.data(), 5, m.data(), v_len);
}

Uint256 Uint256::mulmod(const Uint256& a, const Uint256& b, const Uint256& modulus) noexcept {
  if (modulus.is_zero()) [[unlikely]] {
    return {0};
  }

  // Fast paths for zero operands
  if (a.is_zero() || b.is_zero()) {
    return {0};
  }

  // Compute full 512-bit product: a * b
  std::array<uint64_t, 8> product{};
  umul_512(a.data_.data(), b.data_.data(), product.data());

  // Fast path: product fits in 256 bits
  if ((product[4] | product[5] | product[6] | product[7]) == 0) {
    const Uint256 prod(product[0], product[1], product[2], product[3]);
    if (prod < modulus) {
      return prod;
    }
    return prod % modulus;
  }

  // Divide 512-bit product by 256-bit modulus
  const int v_len = count_significant_words(modulus.data_.data(), 4);
  const int u_len = count_significant_words(product.data(), 8);

  return udivrem_512_256(product.data(), u_len, modulus.data_.data(), v_len);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic,bugprone-easily-swappable-parameters)

}  // namespace div0::types
