#ifndef DIV0_EVM_TRACER_H
#define DIV0_EVM_TRACER_H

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "div0/ethereum/transaction/transaction.h"
#include "div0/evm/block_context.h"
#include "div0/evm/call_frame.h"
#include "div0/evm/execution_result.h"
#include "div0/state/account_provider.h"
#include "div0/state/storage_provider.h"
#include "div0/types/address.h"

namespace div0::evm {

/**
 * @brief Unified tracer interface for EVM execution tracing.
 *
 * Provides hooks for tracing at block, transaction, frame, and opcode levels.
 * Designed for compatibility with Besu's OperationTracer/BlockAwareOperationTracer
 * while using a single unified interface.
 *
 * STATE ACCESS:
 * =============
 * Block and transaction hooks receive direct references to AccountProvider and
 * StorageProvider for state access. This allows tracers to inspect account balances,
 * nonces, and storage values at various points during execution.
 *
 * JNI INTEGRATION:
 * ================
 * For Java interop, the JNITracer implementation creates
 * a combined WorldView Java object from the C++ providers before calling back into
 * Java tracer methods.
 *
 * THREAD SAFETY:
 * ==============
 * Tracer instances should not be shared across threads. Each worker thread should
 * have its own tracer instance (or nullptr for no tracing).
 */
class Tracer {
 public:
  Tracer() = default;
  virtual ~Tracer() = default;

  Tracer(const Tracer&) = delete;
  Tracer(Tracer&&) = delete;
  Tracer& operator=(const Tracer&) = delete;
  Tracer& operator=(Tracer&&) = delete;

  // ===========================================================================
  // BLOCK LIFECYCLE
  // ===========================================================================

  /**
   * @brief Called at the start of block processing.
   *
   * @param accounts Account state at block start
   * @param storage Storage state at block start
   * @param block Block context containing number, timestamp, coinbase, etc.
   * @param miner Address of the block producer
   */
  virtual void trace_start_block(const state::AccountProvider& accounts,
                                 const state::StorageProvider& storage, const BlockContext& block,
                                 const types::Address& miner) = 0;

  /**
   * @brief Called at the end of block processing.
   *
   * @param block Block context
   */
  virtual void trace_end_block(const BlockContext& block) = 0;

  // ===========================================================================
  // TRANSACTION LIFECYCLE
  // ===========================================================================

  /**
   * @brief Called before transaction sender account alteration but after validation.
   *
   * @param accounts Account state before sender changes
   * @param storage Storage state before transaction
   * @param tx Transaction about to be processed
   */
  virtual void trace_prepare_transaction(const state::AccountProvider& accounts,
                                         const state::StorageProvider& storage,
                                         const ethereum::Transaction& tx) = 0;

  /**
   * @brief Called before transaction execution, after sender account alteration.
   *
   * @param accounts Account state after sender nonce/balance changes
   * @param storage Storage state
   * @param tx Transaction about to be executed
   */
  virtual void trace_start_transaction(const state::AccountProvider& accounts,
                                       const state::StorageProvider& storage,
                                       const ethereum::Transaction& tx) = 0;

  /**
   * @brief Called after transaction execution completes.
   *
   * @param accounts Account state after execution
   * @param storage Storage state after execution
   * @param tx Transaction that was executed
   * @param success True if transaction succeeded, false otherwise
   * @param output Return data from the transaction
   * @param logs Logs emitted during execution
   * @param gas_used Total gas consumed
   * @param time_ns Execution time in nanoseconds
   */
  virtual void trace_end_transaction(const state::AccountProvider& accounts,
                                     const state::StorageProvider& storage,
                                     const ethereum::Transaction& tx, bool success,
                                     std::span<const uint8_t> output, const std::vector<Log>& logs,
                                     uint64_t gas_used, uint64_t time_ns) = 0;

  // ===========================================================================
  // FRAME LIFECYCLE
  // ===========================================================================

  /**
   * @brief Called when entering a new call frame (root frame or CALL/CREATE).
   *
   * @param frame The new frame being entered
   */
  virtual void trace_frame_enter(const CallFrame& frame) = 0;

  /**
   * @brief Called when exiting a call frame (RETURN, REVERT, STOP, or error).
   *
   * @param frame The frame being exited
   */
  virtual void trace_frame_exit(const CallFrame& frame) = 0;

  /**
   * @brief Called when re-entering a parent frame after child frame returns.
   *
   * @param frame The parent frame being resumed
   */
  virtual void trace_frame_reenter(const CallFrame& frame) = 0;

  // ===========================================================================
  // PER-OPCODE HOOKS
  // ===========================================================================

  /**
   * @brief Called before each opcode execution.
   *
   * The frame contains pre-execution state (pc, gas, stack, memory).
   * Tracers should capture any needed state here for later use in trace_post_execution.
   *
   * @param frame Current call frame with pre-execution state
   */
  virtual void trace_pre_execution(const CallFrame& frame) = 0;

  /**
   * @brief Called after each opcode execution.
   *
   * @param frame Current call frame (pc has advanced, stack modified)
   * @param status Result of the opcode execution
   * @param gas_cost Gas consumed by this opcode
   */
  virtual void trace_post_execution(const CallFrame& frame, ExecutionStatus status,
                                    uint64_t gas_cost) = 0;

  // ===========================================================================
  // SPECIAL OPERATIONS
  // ===========================================================================

  /**
   * @brief Called when a precompile is executed.
   *
   * @param frame The frame executing the precompile
   * @param gas_required Gas required by the precompile
   * @param output Output data from the precompile
   */
  virtual void trace_precompile_call(const CallFrame& frame, uint64_t gas_required,
                                     std::span<const uint8_t> output) = 0;

  /**
   * @brief Called after a CREATE/CREATE2 operation completes.
   *
   * @param frame The frame that executed the creation
   * @param halt_reason If set, the error that caused creation to fail
   */
  virtual void trace_account_creation_result(const CallFrame& frame,
                                             std::optional<ExecutionStatus> halt_reason) = 0;
};

}  // namespace div0::evm

#endif  // DIV0_EVM_TRACER_H
