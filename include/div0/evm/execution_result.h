#ifndef DIV0_EVM_EXECUTION_RESULT_H
#define DIV0_EVM_EXECUTION_RESULT_H

#include <cstdint>
#include <vector>

#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/uint256.h"

namespace div0::evm {

/**
 * @brief Log emitted by LOG0-LOG4 opcodes.
 *
 * Logs are collected during execution and returned in ExecutionResult.
 */
struct Log {
  types::Address address;  // Contract that emitted the log

  // Indexed event parameters (topics).
  // topic_count indicates how many are valid (0-4).
  // First topic is typically the event signature hash.
  std::vector<types::Uint256> topics;

  // Non-indexed event parameters (ABI-encoded)
  types::Bytes data;

  friend bool operator==(const Log& lhs, const Log& rhs) {
    return lhs.address == rhs.address && lhs.topics == rhs.topics && lhs.data == rhs.data;
  }
  friend bool operator!=(const Log& lhs, const Log& rhs) { return !(lhs == rhs); }
};

/**
 * @brief Status code indicating execution outcome.
 */
enum class ExecutionStatus : uint8_t {
  Success,            // Execution completed normally (STOP, RETURN)
  Revert,             // REVERT opcode - state rolled back, return data available
  OutOfGas,           // Insufficient gas
  InvalidOpcode,      // Undefined or disabled opcode
  StackUnderflow,     // Pop from empty stack
  StackOverflow,      // Stack exceeded 1024 items
  InvalidJump,        // JUMP to non-JUMPDEST
  WriteProtection,    // State modification in STATICCALL context
  CallDepthExceeded,  // Call depth exceeded 1024
};

/**
 * @brief Result of EVM execution.
 *
 * Contains the execution status, gas accounting, return data, and logs.
 */
struct ExecutionResult {
  ExecutionStatus status;

  uint64_t gas_used;    // Gas consumed during execution
  uint64_t gas_refund;  // Gas to refund (e.g., from SSTORE clearing)

  // Output data from RETURN or REVERT opcode.
  // Empty if execution ended via STOP or error.
  types::Bytes return_data;

  // Logs emitted during execution (LOG0-LOG4 opcodes).
  // Only valid if status == Success.
  // Cleared/invalid on Revert or other failures.
  std::vector<Log> logs;

  // Helper to check if execution succeeded
  [[nodiscard]] bool success() const noexcept { return status == ExecutionStatus::Success; }
};

}  // namespace div0::evm

#endif  // DIV0_EVM_EXECUTION_RESULT_H
