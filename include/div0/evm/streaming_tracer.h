#ifndef DIV0_EVM_STREAMING_TRACER_H
#define DIV0_EVM_STREAMING_TRACER_H

#include <ostream>
#include <vector>

#include "div0/evm/tracer.h"
#include "div0/evm/tracer_config.h"
#include "div0/types/uint256.h"

namespace div0::evm {

/**
 * @brief EIP-3155 compatible JSON streaming tracer.
 *
 * Outputs JSON Lines (JSONL) format with one JSON object per line for each
 * executed opcode. Compatible with geth's --trace output format.
 *
 * OUTPUT FORMAT:
 * ==============
 * Each opcode produces a single JSON object (newline-delimited):
 * {"pc":0,"op":96,"gas":"0x5f5e100","gasCost":"0x3","memSize":0,"stack":[],"depth":1,"opName":"PUSH1"}
 *
 * Optional fields (controlled by TracerConfig):
 * - stack: Array of stack values (hex strings) - enabled by default
 * - memory: Hex-encoded memory contents - disabled by default
 * - storage: Storage changes as key-value pairs - disabled by default
 * - returnData: Return data from sub-calls - disabled by default
 *
 * USAGE:
 * ======
 * ```cpp
 * std::ostringstream output;
 * StreamingTracer tracer(output, {.memory = true, .stack = true});
 * evm.set_tracer(&tracer);
 * evm.execute(env);
 * // output.str() now contains JSONL trace
 * ```
 *
 * THREAD SAFETY:
 * ==============
 * Not thread-safe. Each worker thread should have its own StreamingTracer
 * instance writing to its own output stream.
 */
class StreamingTracer : public Tracer {
 public:
  /**
   * @brief Construct a streaming tracer.
   *
   * @param output Output stream for JSON lines (must outlive tracer)
   * @param config Configuration for which fields to include
   */
  explicit StreamingTracer(std::ostream& output, TracerConfig config = {});

  ~StreamingTracer() override = default;

  StreamingTracer(const StreamingTracer&) = delete;
  StreamingTracer(StreamingTracer&&) = delete;
  StreamingTracer& operator=(const StreamingTracer&) = delete;
  StreamingTracer& operator=(StreamingTracer&&) = delete;

  // ===========================================================================
  // BLOCK LIFECYCLE (no-ops for per-opcode tracer)
  // ===========================================================================

  void trace_start_block(const state::AccountProvider& accounts,
                         const state::StorageProvider& storage, const BlockContext& block,
                         const types::Address& miner) override;

  void trace_end_block(const BlockContext& block) override;

  // ===========================================================================
  // TRANSACTION LIFECYCLE (no-ops for per-opcode tracer)
  // ===========================================================================

  void trace_prepare_transaction(const state::AccountProvider& accounts,
                                 const state::StorageProvider& storage,
                                 const ethereum::Transaction& tx) override;

  void trace_start_transaction(const state::AccountProvider& accounts,
                               const state::StorageProvider& storage,
                               const ethereum::Transaction& tx) override;

  void trace_end_transaction(const state::AccountProvider& accounts,
                             const state::StorageProvider& storage, const ethereum::Transaction& tx,
                             bool success, std::span<const uint8_t> output,
                             const std::vector<Log>& logs, uint64_t gas_used,
                             uint64_t time_ns) override;

  // ===========================================================================
  // FRAME LIFECYCLE
  // ===========================================================================

  void trace_frame_enter(const CallFrame& frame) override;
  void trace_frame_exit(const CallFrame& frame) override;
  void trace_frame_reenter(const CallFrame& frame) override;

  // ===========================================================================
  // PER-OPCODE HOOKS
  // ===========================================================================

  /**
   * @brief Called before each opcode execution.
   *
   * Captures pre-execution state for use in trace_post_execution.
   */
  void trace_pre_execution(const CallFrame& frame) override;

  /**
   * @brief Called after each opcode execution.
   *
   * Outputs trace using captured pre-execution state and gas cost.
   */
  void trace_post_execution(const CallFrame& frame, ExecutionStatus status,
                            uint64_t gas_cost) override;

  // ===========================================================================
  // SPECIAL OPERATIONS
  // ===========================================================================

  void trace_precompile_call(const CallFrame& frame, uint64_t gas_required,
                             std::span<const uint8_t> output) override;

  void trace_account_creation_result(const CallFrame& frame,
                                     std::optional<ExecutionStatus> halt_reason) override;

 private:
  /// Write a single JSON trace line for an opcode execution
  void write_trace_line(const CallFrame& frame, uint64_t gas_cost);

  std::ostream& output_;
  TracerConfig config_;

  /// Pre-execution state (captured in trace_pre_execution, used in trace_post_execution)
  uint64_t pre_gas_{0};
  uint64_t pre_pc_{0};
  uint8_t pre_opcode_{0};
  uint64_t pre_mem_size_{0};
  std::vector<types::Uint256> pre_stack_;
};

}  // namespace div0::evm

#endif  // DIV0_EVM_STREAMING_TRACER_H
