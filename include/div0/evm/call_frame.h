#ifndef DIV0_EVM_CALL_FRAME_H
#define DIV0_EVM_CALL_FRAME_H

#include <span>

#include "div0/evm/memory.h"
#include "div0/evm/stack.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::evm {

/// Execution type determines context semantics
enum class ExecutionType : uint8_t {
  TxStart,       // Top-level transaction entry point (root frame)
  Call,          // Regular call - new context, can transfer value
  StaticCall,    // Read-only call - cannot modify state
  DelegateCall,  // Execute callee code in caller's context
  CallCode,      // Legacy: like DelegateCall but different msg.sender
  Create,        // Contract creation via CREATE
  Create2,       // Contract creation via CREATE2
};

/// A single call frame in the EVM execution stack.
/// Hot data (frequently accessed) is packed into the first cache line.
struct alignas(64) CallFrame {
  // === First cache line (64 bytes) - hot data ===

  // offset 0,  8 bytes - program counter
  uint64_t pc{};

  // offset 8,  8 bytes - remaining gas
  uint64_t gas{};

  // offset 16, 8 bytes - operand stack
  Stack* stack{};

  // offset 24, 8 bytes - linear memory
  Memory* memory{};

  // offset 32, 8 bytes - pointer to bytecode
  const uint8_t* code_ptr{};

  // offset 40, 8 bytes - bytecode length
  size_t code_size{};

  // offset 48, 8 bytes - where parent wants return data (in parent's memory)
  uint64_t output_offset{};

  // offset 56, 4 bytes - max return data size parent accepts
  uint32_t output_size{};

  // offset 60, 2 bytes - call depth (max 1024)
  uint16_t depth{};

  // offset 62, 1 byte  - CALL/STATICCALL/DELEGATECALL/etc
  ExecutionType exec_type = ExecutionType::TxStart;

  // offset 63, 1 byte  - static context (no state modifications)
  bool is_static{};

  // === Second cache line - cold data (accessed occasionally) ===
  types::Address caller;   // msg.sender
  types::Address address;  // address(this) - the account whose storage is being accessed
  types::Uint256 value;    // msg.value

  // Input data (call data) - points into parent's memory or original tx input
  const uint8_t* input_ptr{nullptr};
  size_t input_size{0};

  /// Returns the bytecode as a span
  [[nodiscard]] std::span<const uint8_t> code() const noexcept { return {code_ptr, code_size}; }

  /// Returns the input data as a span
  [[nodiscard]] std::span<const uint8_t> input() const noexcept { return {input_ptr, input_size}; }

  /// Reset frame to initial state (for pooling)
  void reset() noexcept {
    pc = 0;
    gas = 0;
    stack = nullptr;
    memory = nullptr;
    code_ptr = nullptr;
    code_size = 0;
    output_offset = 0;
    output_size = 0;
    depth = 0;
    exec_type = ExecutionType::TxStart;
    is_static = false;
    caller = types::Address::zero();
    address = types::Address::zero();
    value = types::Uint256::zero();
    input_ptr = nullptr;
    input_size = 0;
  }
};

static_assert(offsetof(CallFrame, caller) == 64, "Cold data should start at second cache line");

}  // namespace div0::evm

#endif  // DIV0_EVM_CALL_FRAME_H
