#include "div0/evm/streaming_tracer.h"

#include <iomanip>

#include "div0/evm/opcodes.h"

namespace div0::evm {

namespace {

/// Write a hex-encoded Uint256 value to the output stream (no leading zeros, lowercase)
void write_hex_uint256(std::ostream& out, const types::Uint256& value) {
  // Save stream state to avoid pollution from std::hex/std::setfill
  const auto original_flags = out.flags();
  const auto original_fill = out.fill();

  // Find the first non-zero limb (from most significant)
  int first_nonzero = -1;
  for (int i = 3; i >= 0; --i) {
    if (value.limb(static_cast<size_t>(i)) != 0) {
      first_nonzero = i;
      break;
    }
  }

  out << "\"0x";
  if (first_nonzero < 0) {
    out << '0';
  } else {
    // Write first non-zero limb without leading zeros
    out << std::hex << value.limb(static_cast<size_t>(first_nonzero));
    // Write remaining limbs with full 16 hex digits (zero-padded)
    for (int i = first_nonzero - 1; i >= 0; --i) {
      out << std::setfill('0') << std::setw(16) << value.limb(static_cast<size_t>(i));
    }
  }
  out << '"';

  // Restore stream state
  out.flags(original_flags);
  out.fill(original_fill);
}

}  // namespace

StreamingTracer::StreamingTracer(std::ostream& output, TracerConfig config)
    : output_(output), config_(config) {}

// ===========================================================================
// BLOCK LIFECYCLE (no-ops for per-opcode tracer)
// ===========================================================================

void StreamingTracer::trace_start_block(const state::AccountProvider& /*accounts*/,
                                        const state::StorageProvider& /*storage*/,
                                        const BlockContext& /*block*/,
                                        const types::Address& /*miner*/) {
  // No-op for per-opcode tracer
}

void StreamingTracer::trace_end_block(const BlockContext& /*block*/) {
  // No-op for per-opcode tracer
}

// ===========================================================================
// TRANSACTION LIFECYCLE (no-ops for per-opcode tracer)
// ===========================================================================

void StreamingTracer::trace_prepare_transaction(const state::AccountProvider& /*accounts*/,
                                                const state::StorageProvider& /*storage*/,
                                                const ethereum::Transaction& /*tx*/) {
  // No-op for per-opcode tracer
}

void StreamingTracer::trace_start_transaction(const state::AccountProvider& /*accounts*/,
                                              const state::StorageProvider& /*storage*/,
                                              const ethereum::Transaction& /*tx*/) {
  // No-op for per-opcode tracer
}

void StreamingTracer::trace_end_transaction(const state::AccountProvider& /*accounts*/,
                                            const state::StorageProvider& /*storage*/,
                                            const ethereum::Transaction& /*tx*/, bool /*success*/,
                                            std::span<const uint8_t> /*output*/,
                                            const std::vector<Log>& /*logs*/, uint64_t /*gas_used*/,
                                            uint64_t /*time_ns*/) {
  // No-op for per-opcode tracer
}

// ===========================================================================
// FRAME LIFECYCLE
// ===========================================================================

void StreamingTracer::trace_frame_enter(const CallFrame& /*frame*/) {
  // No-op for per-opcode tracer
}

void StreamingTracer::trace_frame_exit(const CallFrame& /*frame*/) {
  // No-op for per-opcode tracer
}

void StreamingTracer::trace_frame_reenter(const CallFrame& /*frame*/) {
  // No-op for per-opcode tracer
}

// ===========================================================================
// PER-OPCODE HOOKS
// ===========================================================================

void StreamingTracer::trace_pre_execution(const CallFrame& frame) {
  // Capture pre-execution state (like Besu's StreamingOperationTracer)
  pre_pc_ = frame.pc;
  pre_gas_ = frame.gas;
  pre_mem_size_ = frame.memory != nullptr ? frame.memory->size() : 0;

  const auto code = frame.code();
  pre_opcode_ = (pre_pc_ < code.size()) ? code[pre_pc_] : 0;

  // Capture stack (from bottom to top, like Besu)
  if (config_.stack && frame.stack != nullptr) {
    const size_t stack_size = frame.stack->size();
    pre_stack_.resize(stack_size);
    for (size_t i = 0; i < stack_size; ++i) {
      // Stack uses depth-from-top indexing (0=top), so stack[size-1-i] reverses to bottom-to-top
      pre_stack_[i] = (*frame.stack)[stack_size - 1 - i];
    }
  } else {
    pre_stack_.clear();
  }
}

void StreamingTracer::trace_post_execution(const CallFrame& frame, ExecutionStatus /*status*/,
                                           uint64_t gas_cost) {
  // Output trace using captured pre-execution state and gas cost from post-execution
  write_trace_line(frame, gas_cost);
}

// ===========================================================================
// SPECIAL OPERATIONS
// ===========================================================================

void StreamingTracer::trace_precompile_call(const CallFrame& /*frame*/, uint64_t /*gas_required*/,
                                            std::span<const uint8_t> /*output*/) {
  // No-op for per-opcode tracer
}

void StreamingTracer::trace_account_creation_result(
    const CallFrame& /*frame*/, std::optional<ExecutionStatus> /*halt_reason*/) {
  // No-op for per-opcode tracer
}

// ===========================================================================
// PRIVATE HELPERS
// ===========================================================================

// NOLINTBEGIN(modernize-raw-string-literal) - JSON format requires escaped quotes

void StreamingTracer::write_trace_line(const CallFrame& frame, uint64_t gas_cost) {
  // EIP-3155 format using PRE-EXECUTION state (captured in trace_pre_execution)
  // {"pc":0,"op":96,"gas":"0x...","gasCost":"0x...","memSize":0,"stack":[...],"depth":1,"opName":"PUSH1"}

  // Save stream state to avoid pollution from std::hex/std::dec/std::setfill
  const auto original_flags = output_.flags();
  const auto original_fill = output_.fill();

  output_ << "{\"pc\":" << pre_pc_;
  output_ << ",\"op\":" << static_cast<unsigned>(pre_opcode_);
  output_ << ",\"gas\":\"0x" << std::hex << pre_gas_ << "\"";
  output_ << ",\"gasCost\":\"0x" << std::hex << gas_cost << "\"";

  // Memory size (pre-execution)
  output_ << std::dec << ",\"memSize\":" << pre_mem_size_;

  // Stack (pre-execution, from bottom to top)
  if (config_.stack) {
    output_ << ",\"stack\":[";
    for (size_t i = 0; i < pre_stack_.size(); ++i) {
      if (i > 0) {
        output_ << ',';
      }
      write_hex_uint256(output_, pre_stack_[i]);
    }
    output_ << ']';
  }

  // Call depth (1-indexed in EIP-3155)
  output_ << ",\"depth\":" << (frame.depth + 1);

  // Refund counter (TODO: track refunds properly in EVM)
  output_ << ",\"refund\":0";

  // Opcode name
  output_ << ",\"opName\":\"" << opcode_name(pre_opcode_) << "\"";

  // Optional: memory contents (TODO: would need pre-execution memory capture)
  // For now, skip since we don't store pre-execution memory

  output_ << "}\n";

  // Restore stream state
  output_.flags(original_flags);
  output_.fill(original_fill);
}

// NOLINTEND(modernize-raw-string-literal)

}  // namespace div0::evm
