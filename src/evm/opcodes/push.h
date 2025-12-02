#ifndef DIV0_EVM_OPCODES_PUSH_H
#define DIV0_EVM_OPCODES_PUSH_H

#include <cassert>
#include <span>

#include "div0/evm/execution_result.h"
#include "div0/evm/stack.h"
#include "div0/types/uint256.h"

namespace div0::evm::opcodes {

/**
 * @brief PUSH0 opcode - push zero (EIP-3855, Shanghai+).
 */
[[gnu::always_inline]] inline ExecutionStatus push0(Stack& stack, uint64_t& gas,
                                                    const uint64_t gas_cost) noexcept {
  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }
  if (!stack.has_space(1)) [[unlikely]] {
    return ExecutionStatus::StackOverflow;
  }

  gas -= gas_cost;

  [[maybe_unused]] const bool push_result = stack.push(types::Uint256(0));
  assert(push_result && "Push should never fail after HasSpace check");

  return ExecutionStatus::Success;
}

// ============================================================================
// Helper: Read N bytes from bytecode into a uint64_t (big-endian)
// ============================================================================
template <size_t N>
[[gnu::always_inline]] uint64_t read_bytes(const std::span<const uint8_t> code, const uint64_t pc,
                                           const size_t to_read) noexcept {
  static_assert(N <= 8, "ReadBytes only handles up to 8 bytes");

  uint64_t value = 0;
  for (size_t i = 0; i < to_read; ++i) {
    value = (value << 8) | code[pc + i];
  }

  // Left-shift to place bytes in MSB positions (big-endian alignment).
  // If we read fewer than N bytes, zeros fill the LSB positions.
  // Guard against shift by 64 (undefined behavior for uint64_t).
  const size_t shift = 8 * (N - to_read);
  return (shift >= 64) ? 0 : (value << shift);
}

// ============================================================================
// Size-specific implementations (compile-time dispatch)
// ============================================================================

// PUSH1-PUSH8: Single limb (fits in data_[0])
template <size_t N>
[[gnu::always_inline]] types::Uint256 construct_single_limb(const std::span<const uint8_t> code,
                                                            const uint64_t pc,
                                                            const size_t to_read) noexcept {
  static_assert(N >= 1 && N <= 8);
  const uint64_t limb0 = read_bytes<N>(code, pc, to_read);
  return types::Uint256(limb0, 0, 0, 0);
}

// PUSH9-PUSH16: Two limbs
template <size_t N>
[[gnu::always_inline]] types::Uint256 construct_two_limbs(std::span<const uint8_t> code,
                                                          const uint64_t pc,
                                                          const size_t to_read) noexcept {
  static_assert(N >= 9 && N <= 16);

  // Layout: limb1=[0, N-8), limb0=[N-8, N)
  // HIGH_BYTES varies: 1 for PUSH9, 8 for PUSH16
  constexpr size_t HIGH_BYTES = N - 8;

  // Truncated bytes go to MSB (limb1 first)
  const size_t l1_read = (to_read < HIGH_BYTES) ? to_read : HIGH_BYTES;

  const size_t after_l1 = (to_read > HIGH_BYTES) ? (to_read - HIGH_BYTES) : 0;
  const size_t l0_read = (after_l1 < 8) ? after_l1 : 8;

  const uint64_t limb1 = read_bytes<HIGH_BYTES>(code, pc, l1_read);
  const uint64_t limb0 = read_bytes<8>(code, pc + HIGH_BYTES, l0_read);

  return types::Uint256(limb0, limb1, 0, 0);
}

// PUSH17-PUSH24: Three limbs
template <size_t N>
[[gnu::always_inline]] types::Uint256 construct_three_limbs(const std::span<const uint8_t> code,
                                                            const uint64_t pc,
                                                            const size_t to_read) noexcept {
  static_assert(N >= 17 && N <= 24);

  // Layout: limb2=[0, N-16), limb1=[N-16, N-8), limb0=[N-8, N)
  // HIGH_BYTES varies: 1 for PUSH17, 8 for PUSH24
  constexpr size_t HIGH_BYTES = N - 16;

  // Truncated bytes go to MSB (limb2 first)
  const size_t l2_read = (to_read < HIGH_BYTES) ? to_read : HIGH_BYTES;

  const size_t after_l2 = (to_read > HIGH_BYTES) ? (to_read - HIGH_BYTES) : 0;
  const size_t l1_read = (after_l2 < 8) ? after_l2 : 8;

  const size_t after_l1 = (after_l2 > 8) ? (after_l2 - 8) : 0;
  const size_t l0_read = (after_l1 < 8) ? after_l1 : 8;

  const uint64_t limb2 = read_bytes<HIGH_BYTES>(code, pc, l2_read);
  const uint64_t limb1 = read_bytes<8>(code, pc + HIGH_BYTES, l1_read);
  const uint64_t limb0 = read_bytes<8>(code, pc + HIGH_BYTES + 8, l0_read);

  return types::Uint256(limb0, limb1, limb2, 0);
}

// PUSH25-PUSH32: Four limbs
template <size_t N>
[[gnu::always_inline]] types::Uint256 construct_four_limbs(const std::span<const uint8_t> code,
                                                           const uint64_t pc,
                                                           const size_t to_read) noexcept {
  static_assert(N >= 25 && N <= 32);

  // Layout: bytes are read big-endian starting at pc
  // For PUSH32: limb3=[0,8), limb2=[8,16), limb1=[16,24), limb0=[24,32)
  // LIMB3_BYTES varies: 1 for PUSH25, 8 for PUSH32
  constexpr size_t LIMB3_BYTES = N - 24;  // 1-8 bytes

  // Calculate how many bytes each limb gets from available data
  // Truncated bytes go to MSB (limb3 first), then limb2, etc.
  const size_t l3_read = (to_read < LIMB3_BYTES) ? to_read : LIMB3_BYTES;

  const size_t after_l3 = (to_read > LIMB3_BYTES) ? (to_read - LIMB3_BYTES) : 0;
  const size_t l2_read = (after_l3 < 8) ? after_l3 : 8;

  const size_t after_l2 = (after_l3 > 8) ? (after_l3 - 8) : 0;
  const size_t l1_read = (after_l2 < 8) ? after_l2 : 8;

  const size_t after_l1 = (after_l2 > 8) ? (after_l2 - 8) : 0;
  const size_t l0_read = (after_l1 < 8) ? after_l1 : 8;

  // Read each limb at its correct position
  const uint64_t limb3 = read_bytes<LIMB3_BYTES>(code, pc, l3_read);
  const uint64_t limb2 = read_bytes<8>(code, pc + LIMB3_BYTES, l2_read);
  const uint64_t limb1 = read_bytes<8>(code, pc + LIMB3_BYTES + 8, l1_read);
  const uint64_t limb0 = read_bytes<8>(code, pc + LIMB3_BYTES + 16, l0_read);

  return types::Uint256(limb0, limb1, limb2, limb3);
}

// ============================================================================
// Main PUSH template - compile-time dispatch to size-specific implementation
// ============================================================================

/**
 * @brief PUSH1-PUSH32 opcode handler.
 *
 * Reads N bytes from bytecode (big-endian). If bytecode is truncated,
 * available bytes fill the MSB positions (zeros in LSB). Advances PC by N.
 *
 * @tparam N Bytes to push (1-32)
 */
template <size_t N>
[[gnu::always_inline]] ExecutionStatus push_n(Stack& stack, uint64_t& gas, const uint64_t gas_cost,
                                              const std::span<const uint8_t> code,
                                              uint64_t& pc) noexcept {
  static_assert(N >= 1 && N <= 32, "PUSH size must be 1-32");

  if (gas < gas_cost) [[unlikely]] {
    return ExecutionStatus::OutOfGas;
  }

  if (!stack.has_space(1)) [[unlikely]] {
    return ExecutionStatus::StackOverflow;
  }

  gas -= gas_cost;

  // Calculate available bytes
  const size_t available = (pc < code.size()) ? (code.size() - pc) : 0;
  const size_t to_read = (available < N) ? available : N;

  // Dispatch to size-specific constructor
  types::Uint256 value;
  if constexpr (N <= 8) {
    value = construct_single_limb<N>(code, pc, to_read);
  } else if constexpr (N <= 16) {
    value = construct_two_limbs<N>(code, pc, to_read);
  } else if constexpr (N <= 24) {
    value = construct_three_limbs<N>(code, pc, to_read);
  } else {
    value = construct_four_limbs<N>(code, pc, to_read);
  }

  [[maybe_unused]] const bool push_result = stack.push(value);
  assert(push_result && "Push should never fail after HasSpace check");

  pc += N;  // Always advance by N, even if bytecode truncated

  return ExecutionStatus::Success;
}

}  // namespace div0::evm::opcodes

#endif  // DIV0_EVM_OPCODES_PUSH_H
