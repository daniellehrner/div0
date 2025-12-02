// Uint256 uses 4-limb array with loop-based indexing for shift operations.
// All indices are bounds-checked by loop conditions.
// Math operations follow standard precedence rules (multiplication before addition).

#ifndef DIV0_TYPES_H
#define DIV0_TYPES_H

#include <array>
#include <cstdint>
#include <span>
#include <utility>

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index,readability-math-missing-parentheses)
namespace div0::types {

/**
 * @brief 256-bit unsigned integer for EVM operations.
 *
 * High-performance implementation using four 64-bit limbs with native CPU
 * intrinsics for carry/borrow propagation and bit manipulation.
 *
 * INTERNAL REPRESENTATION:
 * ========================
 * Values are stored as four 64-bit limbs in little-endian order:
 * - data_[0]: bits 0-63 (least significant)
 * - data_[1]: bits 64-127
 * - data_[2]: bits 128-191
 * - data_[3]: bits 192-255 (most significant)
 *
 * Example: The value 0x123456789ABCDEF0...FEDCBA9876543210 is stored as:
 * @code
 *   data_[0] = 0xFEDCBA9876543210
 *   data_[1] = 0x...
 *   data_[2] = 0x...
 *   data_[3] = 0x123456789ABCDEF0
 * @endcode
 *
 * PERFORMANCE OPTIMIZATIONS:
 * ==========================
 * - **Compiler Intrinsics**: Uses __builtin_addcll/__builtin_subcll for
 *   single-instruction carry/borrow arithmetic
 * - **Cache Alignment**: 32-byte aligned for optimal memory access
 * - **Inline Expansion**: [[gnu::always_inline]] on hot path operations
 *
 * ARITHMETIC SEMANTICS:
 * =====================
 * All operations follow EVM semantics with modular arithmetic (mod 2^256):
 * - Overflow wraps around (e.g., 2^256 - 1 + 1 = 0)
 * - Division by zero returns 0
 * - Shifts >= 256 bits return 0
 *
 * USAGE:
 * ======
 * @code
 *   Uint256 a(100);
 *   Uint256 b(200);
 *   Uint256 sum = a + b;                    // 300
 *   Uint256 product = a * b;                // 20000
 *   bool less = a < b;                      // true
 *   Uint256 shifted = a << Uint256(8);      // 25600
 * @endcode
 *
 * @note Thread-safe: All operations are const or return new values
 */
class Uint256 {
 public:
  /**
   * @brief Default constructor - initializes to zero.
   */
  constexpr Uint256() = default;

  /**
   * @brief Construct from a single 64-bit value.
   *
   * The value is placed in the least significant limb (data_[0]),
   * with all other limbs set to zero.
   *
   * @param value Value for bits 0-63
   */
  constexpr explicit Uint256(const uint64_t value) { data_[0] = value; }

  /**
   * @brief Construct from four 64-bit limbs.
   *
   * Allows direct specification of all 256 bits via four limbs.
   *
   * @param value0 Bits 0-63 (least significant)
   * @param value1 Bits 64-127
   * @param value2 Bits 128-191
   * @param value3 Bits 192-255 (most significant)
   */
  constexpr explicit Uint256(const uint64_t value0, const uint64_t value1, const uint64_t value2,
                             const uint64_t value3) {
    data_[0] = value0;
    data_[1] = value1;
    data_[2] = value2;
    data_[3] = value3;
  }

  constexpr explicit Uint256(const std::array<uint64_t, 4>& value) noexcept {
    data_[0] = value[0];
    data_[1] = value[1];
    data_[2] = value[2];
    data_[3] = value[3];
  }

  constexpr static Uint256 zero() { return Uint256(0); }
  constexpr static Uint256 one() { return Uint256(1); }
  constexpr static Uint256 max() { return Uint256(~0ULL, ~0ULL, ~0ULL, ~0ULL); }

  // ============================================================================
  // Arithmetic Operations
  // ============================================================================

  /**
   * @brief Addition with modular arithmetic (mod 2^256).
   *
   * Uses __builtin_addcll for efficient carry propagation across all four limbs.
   * Overflow wraps around according to EVM semantics.
   *
   * @param other Value to add
   * @return Sum modulo 2^256
   */
  [[gnu::always_inline]] Uint256 operator+(const Uint256& other) const noexcept {
    Uint256 result;
    // Use unsigned long long (not uint64_t) because __builtin_addcll expects
    // unsigned long long* for carry. On Linux x86_64, uint64_t is unsigned long.
    unsigned long long carry = 0;

    // __builtin_addcll handles add with carry in a single CPU instruction
    result.data_[0] = __builtin_addcll(data_[0], other.data_[0], carry, &carry);
    result.data_[1] = __builtin_addcll(data_[1], other.data_[1], carry, &carry);
    result.data_[2] = __builtin_addcll(data_[2], other.data_[2], carry, &carry);
    result.data_[3] = __builtin_addcll(data_[3], other.data_[3], carry, &carry);

    return result;
  }

  /**
   * @brief Subtraction with modular arithmetic (mod 2^256).
   *
   * Uses __builtin_subcll for efficient borrow propagation across all four limbs.
   * Underflow wraps around according to EVM semantics.
   *
   * @param other Value to subtract
   * @return Difference modulo 2^256
   */
  [[gnu::always_inline]] Uint256 operator-(const Uint256& other) const noexcept {
    Uint256 result;
    // Use unsigned long long (not uint64_t) because __builtin_subcll expects
    // unsigned long long* for borrow. On Linux x86_64, uint64_t is unsigned long.
    unsigned long long borrow = 0;

    // __builtin_subcll handles sub with borrow in a single CPU instruction
    result.data_[0] = __builtin_subcll(data_[0], other.data_[0], borrow, &borrow);
    result.data_[1] = __builtin_subcll(data_[1], other.data_[1], borrow, &borrow);
    result.data_[2] = __builtin_subcll(data_[2], other.data_[2], borrow, &borrow);
    result.data_[3] = __builtin_subcll(data_[3], other.data_[3], borrow, &borrow);

    return result;
  }

  /**
   * @brief Multiplication with modular arithmetic (mod 2^256).
   *
   * Optimized schoolbook multiplication using unsigned __int128 for 64x64->128 products.
   * Only computes limbs that contribute to the low 256 bits, skipping high limbs.
   *
   * ALGORITHM:
   * Computes (a0 + a1*B + a2*B² + a3*B³) * (b0 + b1*B + b2*B² + b3*B³) where B = 2^64.
   * Since result is mod 2^256, we only compute products where i+j < 4.
   *
   * @param other Value to multiply by
   * @return Product modulo 2^256
   */
  [[gnu::always_inline]] Uint256 operator*(const Uint256& other) const noexcept {
    using U128 = unsigned __int128;

    // Full result would need 8 limbs (512 bits), but EVM wants mod 2^256,
    // so we only need the low 4 limbs. This means we can skip products
    // that would only contribute to limbs 4-7 (positions where i+j >= 4).
    //
    // Products needed (i+j < 4):
    //   a0*b0 (col 0)
    //   a0*b1, a1*b0 (col 1)
    //   a0*b2, a1*b1, a2*b0 (col 2)
    //   a0*b3, a1*b2, a2*b1, a3*b0 (col 3)
    //
    // Each 64x64 multiply gives a 128-bit result: low 64 goes to column i+j,
    // high 64 carries to column i+j+1.

    // Compute the 6 products that contribute to columns 0-2
    // (we need both halves of these for carry propagation)
    const U128 p00 = static_cast<U128>(data_[0]) * other.data_[0];
    const U128 p01 = static_cast<U128>(data_[0]) * other.data_[1];
    const U128 p10 = static_cast<U128>(data_[1]) * other.data_[0];
    const U128 p02 = static_cast<U128>(data_[0]) * other.data_[2];
    const U128 p11 = static_cast<U128>(data_[1]) * other.data_[1];
    const U128 p20 = static_cast<U128>(data_[2]) * other.data_[0];

    // Column 0: just the low 64 bits of a0*b0
    const auto r0 = static_cast<uint64_t>(p00);

    // Column 1: high(p00) + low(p01) + low(p10)
    // Sum can exceed 64 bits, so use U128 to capture carry
    const U128 col1 = (p00 >> 64) + static_cast<uint64_t>(p01) + static_cast<uint64_t>(p10);
    const auto r1 = static_cast<uint64_t>(col1);

    // Column 2: carry from col1 + high(p01) + high(p10) + low(p02) + low(p11) + low(p20)
    const U128 col2 = (col1 >> 64) + (p01 >> 64) + (p10 >> 64) + static_cast<uint64_t>(p02) +
                      static_cast<uint64_t>(p11) + static_cast<uint64_t>(p20);
    const auto r2 = static_cast<uint64_t>(col2);

    // Column 3: carry from col2 + high(p02) + high(p11) + high(p20)
    //           + a0*b3 + a1*b2 + a2*b1 + a3*b0 (only low 64 bits needed)
    // Since this is the last limb, any overflow is discarded (mod 2^256)
    const auto r3 = static_cast<uint64_t>(col2 >> 64) + static_cast<uint64_t>(p02 >> 64) +
                    static_cast<uint64_t>(p11 >> 64) + static_cast<uint64_t>(p20 >> 64) +
                    (data_[0] * other.data_[3]) + (data_[1] * other.data_[2]) +
                    (data_[2] * other.data_[1]) + (data_[3] * other.data_[0]);

    return Uint256(r0, r1, r2, r3);
  }

  /**
   * @brief Division (EVM semantics: division by zero returns 0).
   *
   * Uses Knuth's Algorithm D for multi-limb division.
   *
   * @param other Divisor
   * @return Quotient (0 if divisor is zero)
   */
  Uint256 operator/(const Uint256& other) const noexcept { return divmod(other).first; }

  /**
   * @brief Modulo operation (EVM semantics: modulo by zero returns 0).
   *
   * Uses Knuth's Algorithm D for multi-limb division.
   *
   * @param other Divisor
   * @return Remainder (0 if divisor is zero)
   */
  Uint256 operator%(const Uint256& other) const noexcept { return divmod(other).second; }

  // ============================================================================
  // Bitwise Operations
  // ============================================================================

  /**
   * @brief Bitwise AND operation.
   *
   * @param other Value to AND with
   * @return Bitwise AND result
   */
  [[gnu::always_inline]] Uint256 operator&(const Uint256& other) const noexcept {
    return Uint256(data_[0] & other.data_[0], data_[1] & other.data_[1], data_[2] & other.data_[2],
                   data_[3] & other.data_[3]);
  }

  /**
   * @brief Bitwise OR operation.
   *
   * @param other Value to OR with
   * @return Bitwise OR result
   */
  [[gnu::always_inline]] Uint256 operator|(const Uint256& other) const noexcept {
    return Uint256(data_[0] | other.data_[0], data_[1] | other.data_[1], data_[2] | other.data_[2],
                   data_[3] | other.data_[3]);
  }

  /**
   * @brief Bitwise XOR operation.
   *
   * @param other Value to XOR with
   * @return Bitwise XOR result
   */
  [[gnu::always_inline]] Uint256 operator^(const Uint256& other) const noexcept {
    return Uint256(data_[0] ^ other.data_[0], data_[1] ^ other.data_[1], data_[2] ^ other.data_[2],
                   data_[3] ^ other.data_[3]);
  }

  /**
   * @brief Bitwise NOT operation (one's complement).
   *
   * Flips all bits in the value.
   *
   * @return Bitwise NOT result
   */
  [[gnu::always_inline]] Uint256 operator~() const noexcept {
    return Uint256(~data_[0], ~data_[1], ~data_[2], ~data_[3]);
  }

  // ============================================================================
  // Shift Operations
  // ============================================================================

  /**
   * @brief Left shift operation (SHL opcode in EVM).
   *
   * Shifts left by N bits, filling with zeros. If shift amount >= 256, returns 0.
   * Handles both limb-aligned and bit-aligned shifts efficiently.
   *
   * @param shift Number of bits to shift (Uint256 to support EVM stack values)
   * @return Left-shifted value (0 if shift >= 256)
   */
  [[gnu::always_inline]] Uint256 operator<<(const Uint256& shift) const noexcept {
    // If any upper limbs are set, or low limb >= 256, result is 0
    if (shift.data_[1] != 0 || shift.data_[2] != 0 || shift.data_[3] != 0 ||
        shift.data_[0] >= 256) {
      return Uint256(0);
    }

    const auto s = static_cast<unsigned>(shift.data_[0]);
    if (s == 0) {
      return *this;
    }

    Uint256 result;
    const unsigned limb_shift = s / 64;
    const unsigned bit_shift = s % 64;

    if (bit_shift == 0) {
      for (unsigned i = limb_shift; i < 4; ++i) {
        result.data_[i] = data_[i - limb_shift];
      }
    } else {
      const unsigned inv_shift = 64 - bit_shift;
      for (unsigned i = limb_shift; i < 4; ++i) {
        result.data_[i] = data_[i - limb_shift] << bit_shift;
        if (i > limb_shift) {
          result.data_[i] |= data_[i - limb_shift - 1] >> inv_shift;
        }
      }
    }

    return result;
  }

  /**
   * @brief Right shift operation (SHR opcode in EVM).
   *
   * Shifts right by N bits, filling with zeros. If shift amount >= 256, returns 0.
   * Handles both limb-aligned and bit-aligned shifts efficiently.
   *
   * @param shift Number of bits to shift (Uint256 to support EVM stack values)
   * @return Right-shifted value (0 if shift >= 256)
   */
  [[gnu::always_inline]] Uint256 operator>>(const Uint256& shift) const noexcept {
    // If any upper limbs are set, or low limb >= 256, result is 0
    if (shift.data_[1] != 0 || shift.data_[2] != 0 || shift.data_[3] != 0 ||
        shift.data_[0] >= 256) {
      return Uint256(0);
    }

    const auto s = static_cast<unsigned>(shift.data_[0]);
    if (s == 0) {
      return *this;
    }

    Uint256 result;
    const unsigned limb_shift = s / 64;
    const unsigned bit_shift = s % 64;

    if (bit_shift == 0) {
      for (unsigned i = 0; i < 4 - limb_shift; ++i) {
        result.data_[i] = data_[i + limb_shift];
      }
    } else {
      const unsigned inv_shift = 64 - bit_shift;
      for (unsigned i = 0; i < 4 - limb_shift; ++i) {
        result.data_[i] = data_[i + limb_shift] >> bit_shift;
        if (i + limb_shift + 1 < 4) {
          result.data_[i] |= data_[i + limb_shift + 1] << inv_shift;
        }
      }
    }

    return result;
  }

  // ============================================================================
  // Comparison Operators
  // ============================================================================

  /**
   * @brief Equality comparison.
   *
   * Compares all four limbs for exact equality.
   *
   * @param other Value to compare with
   * @return true if values are equal, false otherwise
   */
  [[gnu::always_inline]] bool operator==(const Uint256& other) const noexcept {
    return data_[0] == other.data_[0] && data_[1] == other.data_[1] && data_[2] == other.data_[2] &&
           data_[3] == other.data_[3];
  }

  /**
   * @brief Inequality comparison.
   *
   * @param other Value to compare with
   * @return true if values are not equal, false otherwise
   */
  [[gnu::always_inline]] bool operator!=(const Uint256& other) const noexcept {
    return !(*this == other);
  }

  /**
   * @brief Less-than comparison.
   *
   * Compares from most significant limb to least significant,
   * implementing natural unsigned integer ordering.
   *
   * @param other Value to compare with
   * @return true if this < other, false otherwise
   */
  [[gnu::always_inline]] bool operator<(const Uint256& other) const noexcept {
    if (data_[3] != other.data_[3]) {
      return data_[3] < other.data_[3];
    }
    if (data_[2] != other.data_[2]) {
      return data_[2] < other.data_[2];
    }
    if (data_[1] != other.data_[1]) {
      return data_[1] < other.data_[1];
    }
    return data_[0] < other.data_[0];
  }

  /**
   * @brief Greater-than comparison.
   *
   * @param other Value to compare with
   * @return true if this > other, false otherwise
   */
  [[gnu::always_inline]] bool operator>(const Uint256& other) const noexcept {
    if (data_[3] != other.data_[3]) {
      return data_[3] > other.data_[3];
    }
    if (data_[2] != other.data_[2]) {
      return data_[2] > other.data_[2];
    }
    if (data_[1] != other.data_[1]) {
      return data_[1] > other.data_[1];
    }
    return data_[0] > other.data_[0];
  }

  /**
   * @brief Less-than-or-equal comparison.
   *
   * @param other Value to compare with
   * @return true if this <= other, false otherwise
   */
  [[gnu::always_inline]] bool operator<=(const Uint256& other) const noexcept {
    if (data_[3] != other.data_[3]) {
      return data_[3] < other.data_[3];
    }
    if (data_[2] != other.data_[2]) {
      return data_[2] < other.data_[2];
    }
    if (data_[1] != other.data_[1]) {
      return data_[1] < other.data_[1];
    }
    return data_[0] <= other.data_[0];
  }

  /**
   * @brief Greater-than-or-equal comparison.
   *
   * @param other Value to compare with
   * @return true if this >= other, false otherwise
   */
  [[gnu::always_inline]] bool operator>=(const Uint256& other) const noexcept {
    if (data_[3] != other.data_[3]) {
      return data_[3] > other.data_[3];
    }
    if (data_[2] != other.data_[2]) {
      return data_[2] > other.data_[2];
    }
    if (data_[1] != other.data_[1]) {
      return data_[1] > other.data_[1];
    }
    return data_[0] >= other.data_[0];
  }

  /**
   * @brief Count leading zero bits.
   *
   * Uses __builtin_clzll for efficient per-limb counting.
   * Useful for bit position calculations and normalization.
   *
   * @return Number of leading zero bits (0-256)
   * @note Returns 256 if value is zero
   */
  [[nodiscard]] [[gnu::always_inline]] int count_leading_zeros() const noexcept {
    if (data_[3] != 0) {
      return __builtin_clzll(data_[3]);
    }

    if (data_[2] != 0) {
      return 64 + __builtin_clzll(data_[2]);
    }

    if (data_[1] != 0) {
      return 128 + __builtin_clzll(data_[1]);
    }

    if (data_[0] != 0) {
      return 192 + __builtin_clzll(data_[0]);
    }

    return 256;
  }

  /**
   * @brief Check if value is zero.
   *
   * Uses bitwise OR for efficient single-branch check across all limbs.
   *
   * @return true if value is zero, false otherwise
   */
  [[nodiscard]] bool is_zero() const noexcept {
    return (data_[0] | data_[1] | data_[2] | data_[3]) == 0;
  }

  /**
   * @brief Try to convert to uint64_t.
   *
   * Returns true and sets out if the value fits in 64 bits (upper 192 bits are zero).
   * Returns false if the value exceeds uint64_t range.
   *
   * @param out Output parameter for the converted value
   * @return true if conversion succeeded, false if value too large
   */
  [[nodiscard]] [[gnu::always_inline]] bool to_uint64(uint64_t& out) const noexcept {
    if (data_[1] != 0 || data_[2] != 0 || data_[3] != 0) [[unlikely]] {
      return false;
    }
    out = data_[0];
    return true;
  }

  /**
   * @brief Check if value fits in a uint64_t.
   *
   * @return true if upper 192 bits are zero, false otherwise
   */
  [[nodiscard]] [[gnu::always_inline]] constexpr bool fits_uint64() const noexcept {
    return data_[1] == 0 && data_[2] == 0 && data_[3] == 0;
  }

  /**
   * @brief Convert to uint64_t without bounds checking.
   *
   * @warning Caller MUST ensure fits_uint64() returns true before calling this.
   * @return The low 64 bits of the value
   */
  [[nodiscard]] [[gnu::always_inline]] constexpr uint64_t to_uint64_unsafe() const noexcept {
    return data_[0];
  }

  /**
   * @brief Get least significant byte.
   *
   * Returns the low 8 bits of the value.
   *
   * @return Least significant byte (0-255)
   */
  [[nodiscard]] [[gnu::always_inline]] constexpr uint8_t lsb() const noexcept {
    return static_cast<uint8_t>(data_[0] & 0xFF);
  }

  // ============================================================================
  // Signed Arithmetic Operations (Two's Complement Interpretation)
  // ============================================================================

  /**
   * @brief Check if value is negative in two's complement representation.
   *
   * In two's complement, a number is negative if its most significant bit is set.
   * For 256-bit integers, this is bit 255 (the high bit of limb 3).
   *
   * @return true if the high bit (bit 255) is set, false otherwise
   */
  [[nodiscard]] constexpr bool is_negative() const noexcept { return (data_[3] >> 63) != 0; }

  /**
   * @brief Two's complement negation.
   *
   * Computes -x using two's complement: ~x + 1.
   * Note: -0 = 0, and -(MIN_VALUE) = MIN_VALUE (overflow).
   *
   * @return Negated value
   */
  [[nodiscard]] Uint256 negate() const noexcept { return ~(*this) + Uint256(1); }

  /**
   * @brief Signed division (SDIV opcode).
   *
   * Divides two values interpreted as signed two's complement integers.
   * Result follows C99/EVM semantics: sign(a/b) = sign(a) XOR sign(b).
   *
   * Edge cases:
   * - Division by zero returns 0
   * - MIN_VALUE / -1 returns MIN_VALUE (overflow protection)
   *
   * @param a Dividend (signed)
   * @param b Divisor (signed)
   * @return Signed quotient (0 if divisor is zero)
   */
  [[nodiscard]] static Uint256 sdiv(const Uint256& a, const Uint256& b) noexcept;

  /**
   * @brief Signed modulo (SMOD opcode).
   *
   * Computes signed remainder where result sign equals dividend sign.
   * This follows EVM semantics: sign(a % b) = sign(a).
   *
   * Edge cases:
   * - Modulo by zero returns 0
   * - Modulo by 1 or -1 returns 0
   *
   * @param a Dividend (signed)
   * @param b Divisor (signed)
   * @return Signed remainder (0 if divisor is zero)
   */
  [[nodiscard]] static Uint256 smod(const Uint256& a, const Uint256& b) noexcept;

  /**
   * @brief Sign extend from byte position (SIGNEXTEND opcode).
   *
   * Extends the sign bit at the given byte position to all higher bits.
   * The byte position is 0-indexed from the least significant byte.
   * If byte_pos >= 31, the value is unchanged (already fully represented).
   *
   * Example: signextend(0, 0x7F) = 0x7F (positive, no change)
   *          signextend(0, 0x80) = 0xFF...FF80 (sign-extended to negative)
   *
   * @param byte_pos Byte position (0-30) to extend sign from
   * @param x Value to sign-extend
   * @return Sign-extended value
   */
  [[nodiscard]] static Uint256 signextend(const Uint256& byte_pos, const Uint256& x) noexcept;

  /**
   * @brief Exponentiation (EXP opcode).
   *
   * Computes base^exponent mod 2^256 using binary exponentiation (square-and-multiply).
   * Time complexity: O(log exponent) multiplications.
   *
   * @param base Base value
   * @param exponent Exponent value
   * @return base^exponent mod 2^256
   */
  [[nodiscard]] static Uint256 exp(const Uint256& base, const Uint256& exponent) noexcept;

  /**
   * @brief Modular addition (ADDMOD opcode).
   *
   * Computes (a + b) % modulus without intermediate overflow.
   * Uses a 257-bit intermediate representation when addition overflows.
   *
   * Edge cases:
   * - Returns 0 if modulus is zero
   *
   * @param a First addend
   * @param b Second addend
   * @param modulus Modulus value
   * @return (a + b) % modulus
   */
  [[nodiscard]] static Uint256 addmod(const Uint256& a, const Uint256& b,
                                      const Uint256& modulus) noexcept;

  /**
   * @brief Modular multiplication (MULMOD opcode).
   *
   * Computes (a * b) % modulus without intermediate overflow.
   * Uses a 512-bit intermediate representation for the full product.
   *
   * Edge cases:
   * - Returns 0 if modulus is zero
   * - Returns 0 if either a or b is zero
   *
   * @param a First factor
   * @param b Second factor
   * @param modulus Modulus value
   * @return (a * b) % modulus
   */
  [[nodiscard]] static Uint256 mulmod(const Uint256& a, const Uint256& b,
                                      const Uint256& modulus) noexcept;

  /**
   * @brief Get the byte length of the value.
   *
   * Returns the minimum number of bytes needed to represent this value.
   * Used for EXP gas calculation.
   *
   * @return Number of bytes (0-32)
   */
  [[nodiscard]] unsigned byte_length() const noexcept {
    const int lz = count_leading_zeros();
    return static_cast<unsigned>((256 - lz + 7) / 8);
  }

  [[nodiscard]] std::span<const uint64_t, 4> data_unsafe() const noexcept { return data_; }

  /// Access limb by index (0 = least significant)
  [[nodiscard]] constexpr uint64_t limb(size_t i) const noexcept { return data_[i]; }

 private:
  alignas(32) std::array<uint64_t, 4> data_{};

  [[nodiscard]] std::pair<Uint256, Uint256> divmod(const Uint256& divisor) const noexcept;

  [[gnu::always_inline]] Uint256 operator<<(const unsigned shift) const noexcept {
    if (shift >= 256) {
      return Uint256(0);
    }

    if (shift == 0) {
      return *this;
    }

    Uint256 result;
    const unsigned limb_shift = shift / 64;  // How many whole limbs to move
    const unsigned bit_shift = shift % 64;   // Remaining bits

    if (bit_shift == 0) {
      // Exact limb boundary - just move limbs
      for (unsigned i = limb_shift; i < 4; ++i) {
        result.data_[i] = data_[i - limb_shift];
      }
    } else {
      // Need to combine bits from adjacent limbs
      const unsigned inv_shift = 64 - bit_shift;
      for (unsigned i = limb_shift; i < 4; ++i) {
        result.data_[i] = data_[i - limb_shift] << bit_shift;
        if (i > limb_shift) {
          result.data_[i] |= data_[i - limb_shift - 1] >> inv_shift;
        }
      }
    }

    return result;
  }

  [[gnu::always_inline]] Uint256 operator>>(const unsigned shift) const noexcept {
    if (shift >= 256) {
      return Uint256(0);
    }

    if (shift == 0) {
      return *this;
    }

    Uint256 result;
    const unsigned limb_shift = shift / 64;
    const unsigned bit_shift = shift % 64;

    if (bit_shift == 0) {
      for (unsigned i = 0; i < 4 - limb_shift; ++i) {
        result.data_[i] = data_[i + limb_shift];
      }
    } else {
      const unsigned inv_shift = 64 - bit_shift;
      for (unsigned i = 0; i < 4 - limb_shift; ++i) {
        result.data_[i] = data_[i + limb_shift] >> bit_shift;
        if (i + limb_shift + 1 < 4) {
          result.data_[i] |= data_[i + limb_shift + 1] << inv_shift;
        }
      }
    }

    return result;
  }
};

}  // namespace div0::types

// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index,readability-math-missing-parentheses)

#endif  // DIV0_TYPES_H
