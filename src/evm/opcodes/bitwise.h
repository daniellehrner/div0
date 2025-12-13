#ifndef DIV0_EVM_OPCODES_BITWISE_H
#define DIV0_EVM_OPCODES_BITWISE_H

#include "op_helpers.h"

namespace div0::evm::opcodes {

// =============================================================================
// Bitwise Logic Operations
// =============================================================================

[[gnu::always_inline]] inline ExecutionStatus and_(Stack& stack, uint64_t& gas,
                                                   const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& a, const types::Uint256& b) { return a & b; });
}

[[gnu::always_inline]] inline ExecutionStatus or_(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& a, const types::Uint256& b) { return a | b; });
}

[[gnu::always_inline]] inline ExecutionStatus xor_(Stack& stack, uint64_t& gas,
                                                   const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& a, const types::Uint256& b) { return a ^ b; });
}

[[gnu::always_inline]] inline ExecutionStatus not_(Stack& stack, uint64_t& gas,
                                                   const uint64_t gas_cost) noexcept {
  return unary_static_gas_op(stack, gas, gas_cost, [](const types::Uint256& a) { return ~a; });
}

// =============================================================================
// Byte Extraction
// =============================================================================

/// BYTE opcode: Extract a single byte from a 256-bit value
/// Stack: [i, x] -> [x[i]] where i is the byte index (0 = most significant)
[[gnu::always_inline]] inline ExecutionStatus byte(Stack& stack, uint64_t& gas,
                                                   const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& i, const types::Uint256& x) {
        // If i >= 32, result is 0
        if (!i.fits_uint64() || i.to_uint64_unsafe() >= 32) {
          return types::Uint256::zero();
        }

        // EVM uses big-endian byte indexing: byte 0 is the most significant
        // Our Uint256 is little-endian limb order, so we need to calculate:
        // - Which limb contains this byte
        // - Which byte within that limb
        const auto byte_idx = static_cast<unsigned>(i.to_uint64_unsafe());

        // Big-endian index 0 = byte 31 in little-endian (MSB)
        // Big-endian index 31 = byte 0 in little-endian (LSB)
        const unsigned le_byte_idx = 31 - byte_idx;
        const unsigned limb_idx = le_byte_idx / 8;
        const unsigned byte_in_limb = le_byte_idx % 8;

        const uint64_t limb = x.limb(limb_idx);
        const auto byte_val = static_cast<uint8_t>((limb >> (byte_in_limb * 8)) & 0xFF);

        return types::Uint256(byte_val);
      });
}

// =============================================================================
// Shift Operations
// =============================================================================

/// SHL opcode: Shift left
/// Stack: [shift, value] -> [value << shift]
[[gnu::always_inline]] inline ExecutionStatus shl(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost,
      [](const types::Uint256& shift, const types::Uint256& value) { return value << shift; });
}

/// SHR opcode: Logical shift right (zero-fill)
/// Stack: [shift, value] -> [value >> shift]
[[gnu::always_inline]] inline ExecutionStatus shr(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost,
      [](const types::Uint256& shift, const types::Uint256& value) { return value >> shift; });
}

/// SAR opcode: Arithmetic shift right (sign-extending)
/// Stack: [shift, value] -> [value >> shift] with sign extension
[[gnu::always_inline]] inline ExecutionStatus sar(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(
      stack, gas, gas_cost, [](const types::Uint256& shift, const types::Uint256& value) {
        // If shift >= 256, result depends on sign
        if (!shift.fits_uint64() || shift.to_uint64_unsafe() >= 256) {
          // If negative (high bit set), result is all 1s (-1)
          // If non-negative, result is 0
          return value.is_negative() ? types::Uint256::max() : types::Uint256::zero();
        }

        // Perform logical shift
        types::Uint256 result = value >> shift;

        // If the value was negative, fill in the high bits with 1s
        if (value.is_negative()) {
          const auto s = shift.to_uint64_unsafe();
          // Create a mask of s high bits
          // e.g., if s=8, we want bits 248-255 to be 1
          // This is equivalent to ~((1 << (256 - s)) - 1) but we compute
          // it as all 1s shifted left by (256 - s)
          if (s > 0) {
            const types::Uint256 ones = types::Uint256::max();
            const types::Uint256 mask = ones << types::Uint256(256 - s);
            result = result | mask;
          }
        }

        return result;
      });
}

// =============================================================================
// Signed Comparison Operations
// =============================================================================

/// SLT opcode: Signed less-than comparison
/// Stack: [a, b] -> [a <_signed b ? 1 : 0]
[[gnu::always_inline]] inline ExecutionStatus slt(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(stack, gas, gas_cost,
                              [](const types::Uint256& a, const types::Uint256& b) {
                                // In two's complement:
                                // - If a is negative and b is non-negative, a < b
                                // - If a is non-negative and b is negative, a >= b
                                // - If both have same sign, compare as unsigned
                                const bool a_neg = a.is_negative();
                                const bool b_neg = b.is_negative();

                                if (a_neg != b_neg) {
                                  // Different signs: negative < non-negative
                                  return a_neg ? types::Uint256(1) : types::Uint256(0);
                                }
                                // Same sign: unsigned comparison works
                                return (a < b) ? types::Uint256(1) : types::Uint256(0);
                              });
}

/// SGT opcode: Signed greater-than comparison
/// Stack: [a, b] -> [a >_signed b ? 1 : 0]
[[gnu::always_inline]] inline ExecutionStatus sgt(Stack& stack, uint64_t& gas,
                                                  const uint64_t gas_cost) noexcept {
  return binary_static_gas_op(stack, gas, gas_cost,
                              [](const types::Uint256& a, const types::Uint256& b) {
                                const bool a_neg = a.is_negative();
                                const bool b_neg = b.is_negative();

                                if (a_neg != b_neg) {
                                  // Different signs: non-negative > negative
                                  return b_neg ? types::Uint256(1) : types::Uint256(0);
                                }
                                // Same sign: unsigned comparison works
                                return (a > b) ? types::Uint256(1) : types::Uint256(0);
                              });
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_BITWISE_H
