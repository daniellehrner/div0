#ifndef DIV0_EVM_H
#define DIV0_EVM_H

#include <memory>
#include <span>
#include <vector>

#include "div0/evm/call_frame.h"
#include "div0/evm/call_frame_pool.h"
#include "div0/evm/execution_environment.h"
#include "div0/evm/execution_result.h"
#include "div0/evm/forks.h"
#include "div0/evm/frame_result.h"
#include "div0/evm/gas/schedule.h"
#include "div0/evm/memory_pool.h"
#include "div0/evm/stack_pool.h"
#include "div0/state/state_context.h"

namespace div0::evm {

/**
 * @brief Ethereum Virtual Machine interpreter.
 *
 * High-performance EVM implementation using a direct-threaded interpreter
 * (computed goto) for efficient bytecode execution.
 *
 * ARCHITECTURE:
 * =============
 * - **Fork-Aware**: Supports multiple EVM versions (Shanghai, Cancun, Prague)
 * - **Gas Metering**: Accurate gas accounting per EIP specifications
 * - **Stack-Based**: 256-bit word size, 1024 item maximum depth
 * - **Frame-Based**: Nested calls managed via iterative frame loop (not recursive)
 *
 * EXECUTION MODEL:
 * ================
 * The EVM uses an iterative frame management loop:
 * 1. execute() creates root frame and enters loop
 * 2. execute_frame() runs computed-goto interpreter until frame completes or needs child call
 * 3. On CALL/CREATE: loop pushes new frame and continues
 * 4. On RETURN/REVERT/STOP: loop pops frame and resumes parent
 *
 * MEMORY MANAGEMENT:
 * ==================
 * Stack, Memory, and CallFrame objects are rented from per-instance pools
 * to avoid allocation during execution. Pools are pre-allocated (1024 deep
 * each) for zero-allocation hot path.
 *
 * THREAD SAFETY:
 * ==============
 * - Separate EVM instances can be used concurrently from different threads
 * - A single EVM instance must not be used concurrently (no internal locking)
 *
 * @see execute() for bytecode execution
 */
class EVM {
 public:
  /**
   * @brief Construct a new EVM instance.
   *
   * @param state_context Mutable state for storage operations
   * @param fork EVM fork rules to use
   */
  explicit EVM(state::StateContext& state_context, Fork fork);

  /// Destructor
  ~EVM() = default;

  EVM(const EVM& other) = delete;
  EVM(EVM&& other) = delete;
  EVM& operator=(const EVM& other) = delete;
  EVM& operator=(EVM&& other) = delete;

  /**
   * @brief Execute a transaction in the EVM.
   *
   * Runs bytecode using a direct-threaded interpreter with fork-specific
   * opcode support and gas costs. Manages nested CALLs via iterative frame loop.
   *
   * @param env Execution environment containing block, tx, and call context
   * @return ExecutionResult with status, gas usage, return data, and logs
   */
  [[nodiscard]] ExecutionResult execute(const ExecutionEnvironment& env);

 private:
  /**
   * @brief Execute a single call frame until it completes or needs a child call.
   *
   * Contains the computed-goto interpreter loop. Returns when:
   * - Frame completes (STOP, RETURN, REVERT, end of code)
   * - Frame needs child call (CALL, STATICCALL, DELEGATECALL, CREATE, etc.)
   * - Frame errors (OutOfGas, StackUnderflow, InvalidOpcode, etc.)
   *
   * @param frame The call frame to execute
   * @return FrameResult indicating what action the caller should take
   */
  [[nodiscard]] FrameResult execute_frame(CallFrame& frame);

  /**
   * @brief Initialize root call frame from execution environment.
   */
  void init_root_frame(CallFrame& frame, const ExecutionEnvironment& env);

  /**
   * @brief Handle child frame return - copy return data, push success/failure.
   */
  void handle_child_return(const CallFrame& parent, const CallFrame& child,
                           const FrameResult& result);

  /// Return data from last CALL/CREATE (for RETURNDATASIZE/COPY)
  std::vector<uint8_t> return_data_;

  state::StateContext& state_context_;
  Fork fork_;

  /// Current execution environment (set during execute())
  const ExecutionEnvironment* env_{nullptr};

  /// Pending child frame (set by CALL handler, consumed by execute loop)
  CallFrame* pending_frame_{nullptr};

  /// Active dispatch table pointer (fork-specific opcode handlers)
  const void* const* active_dispatch_{nullptr};

  /// Gas cost schedule for current fork
  const gas::Schedule* schedule_{nullptr};

  /// Whether fork-specific dispatch and schedule have been selected
  bool fork_initialized_{false};

  /// Per-instance pools for zero-allocation execution.
  /// Heap-allocated via unique_ptr because pools are ~36MB total (1024 entries each).
  /// Stack allocation would overflow typical 8MB stack limits.
  std::unique_ptr<CallFramePool> frame_pool_;
  std::unique_ptr<StackPool> stack_pool_;
  std::unique_ptr<MemoryPool> memory_pool_;
};

}  // namespace div0::evm

#endif  // DIV0_EVM_H
