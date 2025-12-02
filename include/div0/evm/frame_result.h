#ifndef DIV0_EVM_FRAME_RESULT_H
#define DIV0_EVM_FRAME_RESULT_H

#include <cstdint>

#include "div0/evm/execution_result.h"

namespace div0::evm {

/// Result returned by execute_frame() to signal what action the main loop should take.
/// This is an internal signaling mechanism - not the final ExecutionResult returned to callers.
struct FrameResult {
  enum class Action : uint8_t {
    /// Frame completed successfully (STOP opcode or end of code)
    Stop,
    /// RETURN opcode - frame completed with return data
    Return,
    /// REVERT opcode - frame completed with revert data, state should be rolled back
    Revert,
    /// CALL/STATICCALL/DELEGATECALL/CALLCODE - needs to execute child call
    Call,
    /// CREATE/CREATE2 - needs to execute child contract creation
    Create,
    /// Execution error (OutOfGas, StackUnderflow, InvalidOpcode, etc.)
    Error,
  };

  Action action{Action::Stop};

  /// For Error action: the specific error that occurred
  ExecutionStatus error_status{ExecutionStatus::Success};

  /// For Return/Revert: offset in memory where return data starts
  uint64_t return_offset{0};

  /// For Return/Revert: size of return data in bytes
  uint64_t return_size{0};

  // Factory methods for cleaner construction
  [[nodiscard]] static constexpr FrameResult stop() noexcept { return {.action = Action::Stop}; }

  [[nodiscard]] static constexpr FrameResult return_data(uint64_t offset, uint64_t size) noexcept {
    return {.action = Action::Return, .return_offset = offset, .return_size = size};
  }

  [[nodiscard]] static constexpr FrameResult revert_data(uint64_t offset, uint64_t size) noexcept {
    return {.action = Action::Revert, .return_offset = offset, .return_size = size};
  }

  [[nodiscard]] static constexpr FrameResult call() noexcept { return {.action = Action::Call}; }

  [[nodiscard]] static constexpr FrameResult create() noexcept {
    return {.action = Action::Create};
  }

  [[nodiscard]] static constexpr FrameResult error(ExecutionStatus status) noexcept {
    return {.action = Action::Error, .error_status = status};
  }
};

}  // namespace div0::evm

#endif  // DIV0_EVM_FRAME_RESULT_H
