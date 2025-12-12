#include "div0/evm/evm.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <stack>

#include "div0/evm/gas/dynamic_costs.h"
#include "div0/evm/opcodes.h"
#include "gas/static_costs.h"
#include "opcodes/arithmetic.h"
#include "opcodes/call.h"
#include "opcodes/comparison.h"
#include "opcodes/context.h"
#include "opcodes/dup.h"
#include "opcodes/keccak256.h"
#include "opcodes/memory.h"
#include "opcodes/push.h"
#include "opcodes/storage.h"
#include "opcodes/swap.h"

namespace div0::evm {

EVM::EVM(state::StateContext& state_context, const Fork fork)
    : state_context_(state_context),
      fork_(fork),
      frame_pool_(std::make_unique<CallFramePool>()),
      stack_pool_(std::make_unique<StackPool>()),
      memory_pool_(std::make_unique<MemoryPool>()) {}

void EVM::init_root_frame(CallFrame& frame, const ExecutionEnvironment& env) {
  frame.pc = 0;
  frame.gas = env.call.gas;
  frame.stack = stack_pool_->rent();
  frame.memory = memory_pool_->rent();
  frame.code_ptr = env.call.code.data();
  frame.code_size = env.call.code.size();
  frame.output_offset = 0;
  frame.output_size = 0;
  frame.depth = 0;
  frame.exec_type = ExecutionType::TxStart;
  frame.is_static = env.call.is_static;
  frame.caller = env.call.caller;
  frame.address = env.call.address;
  frame.value = env.call.value;
  frame.input_ptr = env.call.input.data();
  frame.input_size = env.call.input.size();
}

void EVM::handle_child_return(const CallFrame& parent, const CallFrame& child,
                              const FrameResult& result) {
  // Copy return data to return_data_ buffer
  if (result.action == FrameResult::Action::Return ||
      result.action == FrameResult::Action::Revert) {
    if (result.return_size > 0 && child.memory != nullptr) {
      auto span = child.memory->read_span_unsafe(result.return_offset, result.return_size);
      return_data_.assign(span.begin(), span.end());
    } else {
      return_data_.clear();
    }
  } else {
    return_data_.clear();
  }

  // Copy return data to parent's memory at specified output location
  const uint64_t copy_size = std::min(static_cast<uint64_t>(parent.output_size),
                                      static_cast<uint64_t>(return_data_.size()));
  if (copy_size > 0 && parent.memory != nullptr) {
    parent.memory->expand(parent.output_offset + copy_size);
    auto dest = parent.memory->write_span_unsafe(parent.output_offset, copy_size);
    std::copy_n(return_data_.begin(), copy_size, dest.begin());
  }

  // Push success (1) or failure (0) onto parent's stack
  const bool success =
      (result.action == FrameResult::Action::Return || result.action == FrameResult::Action::Stop);
  if (parent.stack != nullptr) {
    (void)parent.stack->push(success ? types::Uint256(1) : types::Uint256::zero());
  }
}

namespace {

/// Extract return data from a frame's memory into a vector
std::vector<uint8_t> extract_return_data(const CallFrame& frame, const FrameResult& result) {
  std::vector<uint8_t> data;
  if (result.return_size > 0 && frame.memory != nullptr) {
    auto span = frame.memory->read_span_unsafe(result.return_offset, result.return_size);
    data.assign(span.begin(), span.end());
  }
  return data;
}

/// Build a successful execution result
ExecutionResult make_success_result(uint64_t gas_used, std::vector<uint8_t> return_data) {
  return {.status = ExecutionStatus::Success,
          .gas_used = gas_used,
          .gas_refund = 0,
          .return_data = std::move(return_data),
          .logs = {}};
}

/// Build a revert execution result
ExecutionResult make_revert_result(uint64_t gas_used, std::vector<uint8_t> revert_data) {
  return {.status = ExecutionStatus::Revert,
          .gas_used = gas_used,
          .gas_refund = 0,
          .return_data = std::move(revert_data),
          .logs = {}};
}

/// Build an error execution result
ExecutionResult make_error_result(ExecutionStatus error_status, uint64_t gas_used) {
  return {
      .status = error_status, .gas_used = gas_used, .gas_refund = 0, .return_data = {}, .logs = {}};
}

}  // namespace

/**
 * @brief Execute a transaction in the EVM using iterative frame management.
 *
 * This is the main entry point for EVM execution. It manages the call frame stack
 * and delegates actual bytecode interpretation to execute_frame().
 *
 * NOTE: The caller is responsible for state transaction management (begin_transaction,
 * commit, rollback) and pre-warming addresses (coinbase, access lists, etc.).
 *
 * DESIGN NOTE: Frame management helpers are defined as lambdas within this function
 * rather than as separate member functions. This design was chosen because:
 *
 * 1. These helpers need to capture and modify local state (frame pointer, frame_stack,
 *    initial_gas). Using member functions would require passing many parameters and
 *    couldn't easily reassign the `frame` pointer from the caller's perspective.
 *
 * 2. The lambdas are opcode-agnostic - they handle frame lifecycle (success, revert,
 *    error, child call/create) without knowing which opcode triggered the transition.
 *    New opcodes (STATICCALL, DELEGATECALL, CREATE, CREATE2) only need changes in
 *    execute_frame() to set up pending_frame_ appropriately; the lambdas here remain
 *    unchanged.
 *
 * 3. Performance is equivalent to inlined code - modern compilers inline small lambdas
 *    with reference captures, and the real cost is in execute_frame()'s interpreter
 *    loop, not frame management.
 *
 * 4. Keeps frame management logic co-located with the main loop that uses it, rather
 *    than scattering implementation details across the class interface.
 */
ExecutionResult EVM::execute(const ExecutionEnvironment& env) {
  env_ = &env;
  const uint64_t initial_gas = env.call.gas;

  // Rent root frame and initialize from environment
  CallFrame* frame = frame_pool_->rent_frame();
  init_root_frame(*frame, env);

  // Trace frame entry for root frame
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_frame_enter(*frame);
  }

  // Frame stack for nested calls (parent frames waiting for child to return)
  std::stack<CallFrame*> frame_stack;

  // Helper to release resources for a single frame
  auto release_frame_resources = [this]() {
    stack_pool_->release();
    memory_pool_->release();
    frame_pool_->return_frame();
  };

  // Handle top-level successful completion (Stop or Return)
  auto handle_top_level_success = [&](const FrameResult& result) -> ExecutionResult {
    if (tracer_ != nullptr) [[unlikely]] {
      tracer_->trace_frame_exit(*frame);
    }
    const uint64_t gas_used = initial_gas - frame->gas;
    auto return_data = (result.action == FrameResult::Action::Return)
                           ? extract_return_data(*frame, result)
                           : std::vector<uint8_t>{};
    release_frame_resources();
    return make_success_result(gas_used, std::move(return_data));
  };

  // Handle top-level revert
  auto handle_top_level_revert = [&](const FrameResult& result) -> ExecutionResult {
    if (tracer_ != nullptr) [[unlikely]] {
      tracer_->trace_frame_exit(*frame);
    }
    const uint64_t gas_used = initial_gas - frame->gas;
    auto revert_data = extract_return_data(*frame, result);
    release_frame_resources();
    return make_revert_result(gas_used, std::move(revert_data));
  };

  // Handle top-level error
  auto handle_top_level_error = [&](const FrameResult& result) -> ExecutionResult {
    if (tracer_ != nullptr) [[unlikely]] {
      tracer_->trace_frame_exit(*frame);
    }
    const uint64_t gas_used = initial_gas - frame->gas;
    release_frame_resources();
    return make_error_result(result.error_status, gas_used);
  };

  // Handle child frame completion (success or revert) - returns to parent
  auto handle_child_completion = [&](const FrameResult& result) {
    // Trace child frame exit before releasing resources
    if (tracer_ != nullptr) [[unlikely]] {
      tracer_->trace_frame_exit(*frame);
    }
    const CallFrame* child = frame;
    frame = frame_stack.top();
    frame_stack.pop();
    frame->gas += child->gas;  // Refund unused gas
    handle_child_return(*frame, *child, result);
    release_frame_resources();
    // Trace parent frame reenter after child returns
    if (tracer_ != nullptr) [[unlikely]] {
      tracer_->trace_frame_reenter(*frame);
    }
  };

  // Handle child frame error - returns to parent with failure
  auto handle_child_error = [&]() {
    // Trace child frame exit before releasing resources
    if (tracer_ != nullptr) [[unlikely]] {
      tracer_->trace_frame_exit(*frame);
    }
    frame = frame_stack.top();
    frame_stack.pop();
    // Child consumed all its gas on error (no refund)
    return_data_.clear();
    if (frame->stack != nullptr) {
      (void)frame->stack->push(types::Uint256::zero());
    }
    release_frame_resources();
    // Trace parent frame reenter after child error
    if (tracer_ != nullptr) [[unlikely]] {
      tracer_->trace_frame_reenter(*frame);
    }
  };

  // Handle pending child call/create
  auto handle_pending_call = [&]() -> std::optional<ExecutionResult> {
    if (pending_frame_ == nullptr) {
      // Should never happen - indicates bug in CALL handler
      release_frame_resources();
      return make_error_result(ExecutionStatus::InvalidOpcode, initial_gas - frame->gas);
    }
    frame_stack.push(frame);
    frame = pending_frame_;
    pending_frame_ = nullptr;
    // Trace child frame entry
    if (tracer_ != nullptr) [[unlikely]] {
      tracer_->trace_frame_enter(*frame);
    }
    return std::nullopt;
  };

  // Main execution loop
  while (true) {
    const FrameResult result = execute_frame(*frame);

    switch (result.action) {
      case FrameResult::Action::Stop:
      case FrameResult::Action::Return:
        if (frame->depth == 0) {
          return handle_top_level_success(result);
        }
        handle_child_completion(result);
        break;

      case FrameResult::Action::Revert:
        if (frame->depth == 0) {
          return handle_top_level_revert(result);
        }
        // TODO: Rollback state changes from child
        handle_child_completion(result);
        break;

      case FrameResult::Action::Call:
      case FrameResult::Action::Create:
        if (auto error = handle_pending_call()) {
          return *error;
        }
        break;

      case FrameResult::Action::Error:
        if (frame->depth == 0) {
          return handle_top_level_error(result);
        }
        handle_child_error();
        break;
    }
  }
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-pro-bounds-pointer-arithmetic,readability-function-size)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-label-as-value"

/**
 * @brief Execute a single call frame using direct-threaded interpreter.
 *
 * This function contains the computed-goto interpreter loop. It runs until:
 * - Frame completes (STOP, RETURN, REVERT, end of code)
 * - Frame needs child call (CALL, STATICCALL, DELEGATECALL, CREATE, etc.)
 * - Frame errors (OutOfGas, StackUnderflow, InvalidOpcode, etc.)
 *
 * IMPLEMENTATION TECHNIQUE: Computed Goto (Direct-Threading)
 * =========================================================
 * Uses GCC/Clang's "labels as values" extension to eliminate switch overhead.
 * Each opcode handler jumps directly to the next handler.
 */
FrameResult EVM::execute_frame(CallFrame& frame) {
  // Thread-safe dispatch table and schedule initialization using double-checked locking
  static std::atomic dispatches_initialized{false};
  static std::mutex init_mutex;
  static std::array<const void*, 256> dispatch_shanghai;
  static std::array<const void*, 256> dispatch_cancun;
  static std::array<const void*, 256> dispatch_prague;
  static gas::Schedule schedule_shanghai;
  static gas::Schedule schedule_cancun;
  static gas::Schedule schedule_prague;

  // Local references for hot path performance
  Stack& stack = *frame.stack;
  Memory& memory = *frame.memory;
  uint64_t& gas = frame.gas;
  uint64_t& pc = frame.pc;
  const auto code = frame.code();

  ExecutionStatus status{};

  // Double-checked locking: fast path avoids mutex when already initialized
  if (!dispatches_initialized.load(std::memory_order_acquire)) {
    const std::scoped_lock lock(init_mutex);
    // Check again after acquiring lock (another thread may have initialized)
    if (!dispatches_initialized.load(std::memory_order_relaxed)) {
      // Initialize Shanghai (base fork)
      dispatch_shanghai.fill(&&op_invalid);
      dispatch_shanghai[op::STOP] = &&op_stop;
      dispatch_shanghai[op::ADD] = &&op_add;
      dispatch_shanghai[op::MUL] = &&op_mul;
      dispatch_shanghai[op::SUB] = &&op_sub;
      dispatch_shanghai[op::DIV] = &&op_div;
      dispatch_shanghai[op::SDIV] = &&op_sdiv;
      dispatch_shanghai[op::MOD] = &&op_mod;
      dispatch_shanghai[op::SMOD] = &&op_smod;
      dispatch_shanghai[op::ADDMOD] = &&op_addmod;
      dispatch_shanghai[op::MULMOD] = &&op_mulmod;
      dispatch_shanghai[op::EXP] = &&op_exp;
      dispatch_shanghai[op::SIGNEXTEND] = &&op_signextend;
      dispatch_shanghai[op::LT] = &&op_lt;
      dispatch_shanghai[op::GT] = &&op_gt;
      dispatch_shanghai[op::EQ] = &&op_eq;
      dispatch_shanghai[op::ISZERO] = &&op_is_zero;
      dispatch_shanghai[op::KECCAK256] = &&op_sha3;
      dispatch_shanghai[op::CALLER] = &&op_caller;
      dispatch_shanghai[op::MLOAD] = &&op_mload;
      dispatch_shanghai[op::MSTORE] = &&op_mstore;
      dispatch_shanghai[op::MSTORE8] = &&op_mstore8;
      dispatch_shanghai[op::SLOAD] = &&op_sload;
      dispatch_shanghai[op::SSTORE] = &&op_sstore;
      dispatch_shanghai[op::MSIZE] = &&op_msize;
      dispatch_shanghai[op::PUSH0] = &&op_push0;
      dispatch_shanghai[op::PUSH1] = &&op_push1;
      dispatch_shanghai[op::PUSH2] = &&op_push2;
      dispatch_shanghai[op::PUSH3] = &&op_push3;
      dispatch_shanghai[op::PUSH4] = &&op_push4;
      dispatch_shanghai[op::PUSH5] = &&op_push5;
      dispatch_shanghai[op::PUSH6] = &&op_push6;
      dispatch_shanghai[op::PUSH7] = &&op_push7;
      dispatch_shanghai[op::PUSH8] = &&op_push8;
      dispatch_shanghai[op::PUSH9] = &&op_push9;
      dispatch_shanghai[op::PUSH10] = &&op_push10;
      dispatch_shanghai[op::PUSH11] = &&op_push11;
      dispatch_shanghai[op::PUSH12] = &&op_push12;
      dispatch_shanghai[op::PUSH13] = &&op_push13;
      dispatch_shanghai[op::PUSH14] = &&op_push14;
      dispatch_shanghai[op::PUSH15] = &&op_push15;
      dispatch_shanghai[op::PUSH16] = &&op_push16;
      dispatch_shanghai[op::PUSH17] = &&op_push17;
      dispatch_shanghai[op::PUSH18] = &&op_push18;
      dispatch_shanghai[op::PUSH19] = &&op_push19;
      dispatch_shanghai[op::PUSH20] = &&op_push20;
      dispatch_shanghai[op::PUSH21] = &&op_push21;
      dispatch_shanghai[op::PUSH22] = &&op_push22;
      dispatch_shanghai[op::PUSH23] = &&op_push23;
      dispatch_shanghai[op::PUSH24] = &&op_push24;
      dispatch_shanghai[op::PUSH25] = &&op_push25;
      dispatch_shanghai[op::PUSH26] = &&op_push26;
      dispatch_shanghai[op::PUSH27] = &&op_push27;
      dispatch_shanghai[op::PUSH28] = &&op_push28;
      dispatch_shanghai[op::PUSH29] = &&op_push29;
      dispatch_shanghai[op::PUSH30] = &&op_push30;
      dispatch_shanghai[op::PUSH31] = &&op_push31;
      dispatch_shanghai[op::PUSH32] = &&op_push32;
      dispatch_shanghai[op::DUP1] = &&op_dup1;
      dispatch_shanghai[op::DUP2] = &&op_dup2;
      dispatch_shanghai[op::DUP3] = &&op_dup3;
      dispatch_shanghai[op::DUP4] = &&op_dup4;
      dispatch_shanghai[op::DUP5] = &&op_dup5;
      dispatch_shanghai[op::DUP6] = &&op_dup6;
      dispatch_shanghai[op::DUP7] = &&op_dup7;
      dispatch_shanghai[op::DUP8] = &&op_dup8;
      dispatch_shanghai[op::DUP9] = &&op_dup9;
      dispatch_shanghai[op::DUP10] = &&op_dup10;
      dispatch_shanghai[op::DUP11] = &&op_dup11;
      dispatch_shanghai[op::DUP12] = &&op_dup12;
      dispatch_shanghai[op::DUP13] = &&op_dup13;
      dispatch_shanghai[op::DUP14] = &&op_dup14;
      dispatch_shanghai[op::DUP15] = &&op_dup15;
      dispatch_shanghai[op::DUP16] = &&op_dup16;
      dispatch_shanghai[op::SWAP1] = &&op_swap1;
      dispatch_shanghai[op::SWAP2] = &&op_swap2;
      dispatch_shanghai[op::SWAP3] = &&op_swap3;
      dispatch_shanghai[op::SWAP4] = &&op_swap4;
      dispatch_shanghai[op::SWAP5] = &&op_swap5;
      dispatch_shanghai[op::SWAP6] = &&op_swap6;
      dispatch_shanghai[op::SWAP7] = &&op_swap7;
      dispatch_shanghai[op::SWAP8] = &&op_swap8;
      dispatch_shanghai[op::SWAP9] = &&op_swap9;
      dispatch_shanghai[op::SWAP10] = &&op_swap10;
      dispatch_shanghai[op::SWAP11] = &&op_swap11;
      dispatch_shanghai[op::SWAP12] = &&op_swap12;
      dispatch_shanghai[op::SWAP13] = &&op_swap13;
      dispatch_shanghai[op::SWAP14] = &&op_swap14;
      dispatch_shanghai[op::SWAP15] = &&op_swap15;
      dispatch_shanghai[op::SWAP16] = &&op_swap16;
      dispatch_shanghai[op::CALL] = &&op_call;
      dispatch_shanghai[op::CALLCODE] = &&op_callcode;
      dispatch_shanghai[op::DELEGATECALL] = &&op_delegatecall;
      dispatch_shanghai[op::STATICCALL] = &&op_staticcall;
      dispatch_shanghai[op::RETURN] = &&op_return;
      dispatch_shanghai[op::REVERT] = &&op_revert;

      // Cancun inherits Shanghai opcodes
      dispatch_cancun = dispatch_shanghai;
      // TODO: Add Cancun-specific opcodes here

      // Prague inherits Cancun opcodes
      dispatch_prague = dispatch_cancun;
      // TODO: Add Prague-specific opcodes here

      // Initialize Shanghai gas schedule (base fork)
      schedule_shanghai = {
          .static_costs = gas::StaticCosts::SHANGHAI,
          .sload = gas::sload::eip2929,
          .sstore = gas::sstore::eip2929,
          .exp = gas::exp::eip160,
          .memory_access = gas::memory::access_cost,
          .sha3_word_cost = 6,
          .tx_base_cost = 21000,
      };

      // Cancun inherits Shanghai schedule
      schedule_cancun = schedule_shanghai;
      schedule_cancun.static_costs = gas::StaticCosts::CANCUN;

      // Prague inherits Cancun schedule
      schedule_prague = schedule_cancun;
      schedule_prague.static_costs = gas::StaticCosts::PRAGUE;

      // Release barrier ensures all table writes are visible before flag is set
      dispatches_initialized.store(true, std::memory_order_release);
    }
  }

  if (!fork_initialized_) {
    switch (fork_) {
      case Fork::Shanghai:
        active_dispatch_ = dispatch_shanghai.data();
        schedule_ = &schedule_shanghai;
        break;
      case Fork::Cancun:
        active_dispatch_ = dispatch_cancun.data();
        schedule_ = &schedule_cancun;
        break;
      case Fork::Prague:
        active_dispatch_ = dispatch_prague.data();
        schedule_ = &schedule_prague;
        break;
    }
    fork_initialized_ = true;
  }

  // Bounds check: ensure code is not empty
  if (pc >= code.size()) [[unlikely]] {
    return FrameResult::stop();
  }

// Macro to dispatch to next instruction
#define DISPATCH_NEXT()                 \
  if (pc >= code.size()) [[unlikely]] { \
    return FrameResult::stop();         \
  }                                     \
  goto* active_dispatch_[code[pc++]]

// Macro for traced opcode handlers with static gas cost
#define OPCODE_HANDLER(name, func, opcode_const)                         \
  op_##name : {                                                          \
    const uint64_t gas_cost = schedule_->static_costs[op::opcode_const]; \
    if (tracer_ != nullptr) [[unlikely]] {                               \
      tracer_->trace_pre_execution(frame);                               \
    }                                                                    \
    status = func(stack, gas, gas_cost);                                 \
    if (tracer_ != nullptr) [[unlikely]] {                               \
      tracer_->trace_post_execution(frame, status, gas_cost);            \
    }                                                                    \
    if (status != ExecutionStatus::Success) [[unlikely]] {               \
      return FrameResult::error(status);                                 \
    }                                                                    \
  }                                                                      \
  DISPATCH_NEXT();

  // Start execution
  DISPATCH_NEXT();

op_stop:
  return FrameResult::stop();

  // Arithmetic opcodes
  OPCODE_HANDLER(add, opcodes::add, ADD)
  OPCODE_HANDLER(mul, opcodes::mul, MUL)
  OPCODE_HANDLER(sub, opcodes::sub, SUB)
  OPCODE_HANDLER(div, opcodes::div, DIV)
  OPCODE_HANDLER(sdiv, opcodes::sdiv, SDIV)
  OPCODE_HANDLER(mod, opcodes::mod, MOD)
  OPCODE_HANDLER(smod, opcodes::smod, SMOD)
  OPCODE_HANDLER(addmod, opcodes::addmod, ADDMOD)
  OPCODE_HANDLER(mulmod, opcodes::mulmod, MULMOD)
  OPCODE_HANDLER(signextend, opcodes::signextend, SIGNEXTEND)

op_exp: {
  // EXP has dynamic gas cost, computed inside the opcode
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_pre_execution(frame);
  }
  const uint64_t gas_before = gas;
  status = opcodes::exp(stack, gas, schedule_->exp);
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_post_execution(frame, status, gas_before - gas);
  }
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
}
  DISPATCH_NEXT();

  // Comparison opcodes
  OPCODE_HANDLER(lt, opcodes::lt, LT)
  OPCODE_HANDLER(gt, opcodes::gt, GT)
  OPCODE_HANDLER(eq, opcodes::eq, EQ)
  OPCODE_HANDLER(is_zero, opcodes::is_zero, ISZERO)

op_sha3: {
  // SHA3 has dynamic gas cost (base + per-word + memory expansion)
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_pre_execution(frame);
  }
  const uint64_t gas_before = gas;
  status = opcodes::keccak256(stack, gas, schedule_->static_costs[op::KECCAK256],
                              schedule_->sha3_word_cost, schedule_->memory_access, memory);
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_post_execution(frame, status, gas_before - gas);
  }
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
}
  DISPATCH_NEXT();

  // CALLER opcode - push msg.sender onto stack
op_caller: {
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_pre_execution(frame);
  }
  const uint64_t gas_cost = schedule_->static_costs[op::CALLER];
  status = opcodes::caller(stack, gas, gas_cost, frame.caller);
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_post_execution(frame, status, gas_cost);
  }
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
}
  DISPATCH_NEXT();

// Memory opcodes have dynamic gas (base + expansion)
#define MEMORY_OPCODE_HANDLER(name, func, opcode_const)                                            \
  op_##name : {                                                                                    \
    if (tracer_ != nullptr) [[unlikely]] {                                                         \
      tracer_->trace_pre_execution(frame);                                                         \
    }                                                                                              \
    const uint64_t gas_before = gas;                                                               \
    status = func(stack, gas, schedule_->static_costs[op::opcode_const], schedule_->memory_access, \
                  memory);                                                                         \
    if (tracer_ != nullptr) [[unlikely]] {                                                         \
      tracer_->trace_post_execution(frame, status, gas_before - gas);                              \
    }                                                                                              \
    if (status != ExecutionStatus::Success) [[unlikely]] {                                         \
      return FrameResult::error(status);                                                           \
    }                                                                                              \
  }                                                                                                \
  DISPATCH_NEXT();

  MEMORY_OPCODE_HANDLER(mload, opcodes::mload, MLOAD)
  MEMORY_OPCODE_HANDLER(mstore, opcodes::mstore, MSTORE)
  MEMORY_OPCODE_HANDLER(mstore8, opcodes::mstore8, MSTORE8)

#undef MEMORY_OPCODE_HANDLER

op_msize: {
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_pre_execution(frame);
  }
  const uint64_t gas_cost = schedule_->static_costs[op::MSIZE];
  status = opcodes::msize(stack, gas, gas_cost, memory);
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_post_execution(frame, status, gas_cost);
  }
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
}
  DISPATCH_NEXT();

op_sload: {
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_pre_execution(frame);
  }
  // SLOAD has dynamic gas cost (cold vs warm access)
  const uint64_t gas_before = gas;
  status = opcodes::sload(frame.address, stack, gas, state_context_.storage, schedule_->sload);
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_post_execution(frame, status, gas_before - gas);
  }
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
}
  DISPATCH_NEXT();

op_sstore: {
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_pre_execution(frame);
  }
  if (frame.is_static) [[unlikely]] {
    return FrameResult::error(ExecutionStatus::WriteProtection);
  }
  // SSTORE has dynamic gas cost (cold vs warm, original vs current value)
  const uint64_t gas_before = gas;
  status = opcodes::sstore(frame.address, stack, gas, state_context_.storage, schedule_->sstore);
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_post_execution(frame, status, gas_before - gas);
  }
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
}
  DISPATCH_NEXT();

  // PUSH0: push zero onto stack
  OPCODE_HANDLER(push0, opcodes::push0, PUSH0)

// PUSH1-32: push N bytes from bytecode onto stack
#define PUSH_N_HANDLER(n, opcode_const)                                  \
  op_push##n : {                                                         \
    if (tracer_ != nullptr) [[unlikely]] {                               \
      tracer_->trace_pre_execution(frame);                               \
    }                                                                    \
    const uint64_t gas_cost = schedule_->static_costs[op::opcode_const]; \
    status = opcodes::push_n<n>(stack, gas, gas_cost, code, pc);         \
    if (tracer_ != nullptr) [[unlikely]] {                               \
      tracer_->trace_post_execution(frame, status, gas_cost);            \
    }                                                                    \
    if (status != ExecutionStatus::Success) [[unlikely]] {               \
      return FrameResult::error(status);                                 \
    }                                                                    \
  }                                                                      \
  DISPATCH_NEXT();

  PUSH_N_HANDLER(1, PUSH1)
  PUSH_N_HANDLER(2, PUSH2)
  PUSH_N_HANDLER(3, PUSH3)
  PUSH_N_HANDLER(4, PUSH4)
  PUSH_N_HANDLER(5, PUSH5)
  PUSH_N_HANDLER(6, PUSH6)
  PUSH_N_HANDLER(7, PUSH7)
  PUSH_N_HANDLER(8, PUSH8)
  PUSH_N_HANDLER(9, PUSH9)
  PUSH_N_HANDLER(10, PUSH10)
  PUSH_N_HANDLER(11, PUSH11)
  PUSH_N_HANDLER(12, PUSH12)
  PUSH_N_HANDLER(13, PUSH13)
  PUSH_N_HANDLER(14, PUSH14)
  PUSH_N_HANDLER(15, PUSH15)
  PUSH_N_HANDLER(16, PUSH16)
  PUSH_N_HANDLER(17, PUSH17)
  PUSH_N_HANDLER(18, PUSH18)
  PUSH_N_HANDLER(19, PUSH19)
  PUSH_N_HANDLER(20, PUSH20)
  PUSH_N_HANDLER(21, PUSH21)
  PUSH_N_HANDLER(22, PUSH22)
  PUSH_N_HANDLER(23, PUSH23)
  PUSH_N_HANDLER(24, PUSH24)
  PUSH_N_HANDLER(25, PUSH25)
  PUSH_N_HANDLER(26, PUSH26)
  PUSH_N_HANDLER(27, PUSH27)
  PUSH_N_HANDLER(28, PUSH28)
  PUSH_N_HANDLER(29, PUSH29)
  PUSH_N_HANDLER(30, PUSH30)
  PUSH_N_HANDLER(31, PUSH31)
  PUSH_N_HANDLER(32, PUSH32)

#undef PUSH_N_HANDLER

#define DUP_N_HANDLER(n, opcode_const)                                   \
  op_dup##n : {                                                          \
    if (tracer_ != nullptr) [[unlikely]] {                               \
      tracer_->trace_pre_execution(frame);                               \
    }                                                                    \
    const uint64_t gas_cost = schedule_->static_costs[op::opcode_const]; \
    status = opcodes::dup_n(stack, gas, gas_cost, n);                    \
    if (tracer_ != nullptr) [[unlikely]] {                               \
      tracer_->trace_post_execution(frame, status, gas_cost);            \
    }                                                                    \
    if (status != ExecutionStatus::Success) [[unlikely]] {               \
      return FrameResult::error(status);                                 \
    }                                                                    \
  }                                                                      \
  DISPATCH_NEXT();

  DUP_N_HANDLER(1, DUP1)
  DUP_N_HANDLER(2, DUP2)
  DUP_N_HANDLER(3, DUP3)
  DUP_N_HANDLER(4, DUP4)
  DUP_N_HANDLER(5, DUP5)
  DUP_N_HANDLER(6, DUP6)
  DUP_N_HANDLER(7, DUP7)
  DUP_N_HANDLER(8, DUP8)
  DUP_N_HANDLER(9, DUP9)
  DUP_N_HANDLER(10, DUP10)
  DUP_N_HANDLER(11, DUP11)
  DUP_N_HANDLER(12, DUP12)
  DUP_N_HANDLER(13, DUP13)
  DUP_N_HANDLER(14, DUP14)
  DUP_N_HANDLER(15, DUP15)
  DUP_N_HANDLER(16, DUP16)

#undef DUP_N_HANDLER

#define SWAP_N_HANDLER(n, opcode_const)                                  \
  op_swap##n : {                                                         \
    if (tracer_ != nullptr) [[unlikely]] {                               \
      tracer_->trace_pre_execution(frame);                               \
    }                                                                    \
    const uint64_t gas_cost = schedule_->static_costs[op::opcode_const]; \
    status = opcodes::swap_n(stack, gas, gas_cost, n);                   \
    if (tracer_ != nullptr) [[unlikely]] {                               \
      tracer_->trace_post_execution(frame, status, gas_cost);            \
    }                                                                    \
    if (status != ExecutionStatus::Success) [[unlikely]] {               \
      return FrameResult::error(status);                                 \
    }                                                                    \
  }                                                                      \
  DISPATCH_NEXT();

  SWAP_N_HANDLER(1, SWAP1)
  SWAP_N_HANDLER(2, SWAP2)
  SWAP_N_HANDLER(3, SWAP3)
  SWAP_N_HANDLER(4, SWAP4)
  SWAP_N_HANDLER(5, SWAP5)
  SWAP_N_HANDLER(6, SWAP6)
  SWAP_N_HANDLER(7, SWAP7)
  SWAP_N_HANDLER(8, SWAP8)
  SWAP_N_HANDLER(9, SWAP9)
  SWAP_N_HANDLER(10, SWAP10)
  SWAP_N_HANDLER(11, SWAP11)
  SWAP_N_HANDLER(12, SWAP12)
  SWAP_N_HANDLER(13, SWAP13)
  SWAP_N_HANDLER(14, SWAP14)
  SWAP_N_HANDLER(15, SWAP15)
  SWAP_N_HANDLER(16, SWAP16)

#undef SWAP_N_HANDLER

// Macro for CALL family opcodes - handles tracing, error checking, and frame setup
// Differences: prepare_fn, exec_type, is_static, caller, address, value
#define CALL_OPCODE_HANDLER(label, prepare_call_expr, exec_type_val, is_static_expr, caller_expr, \
                            address_expr, value_expr)                                             \
  label: {                                                                                        \
    if (tracer_ != nullptr) [[unlikely]] {                                                        \
      tracer_->trace_pre_execution(frame);                                                        \
    }                                                                                             \
    const uint64_t gas_before = gas;                                                              \
    auto setup = prepare_call_expr;                                                               \
                                                                                                  \
    if (setup.status == ExecutionStatus::CallDepthExceeded) [[unlikely]] {                        \
      if (tracer_ != nullptr) [[unlikely]] {                                                      \
        tracer_->trace_post_execution(frame, setup.status, gas_before - gas);                     \
      }                                                                                           \
      DISPATCH_NEXT();                                                                            \
    }                                                                                             \
                                                                                                  \
    if (setup.status != ExecutionStatus::Success) [[unlikely]] {                                  \
      if (tracer_ != nullptr) [[unlikely]] {                                                      \
        tracer_->trace_post_execution(frame, setup.status, gas_before - gas);                     \
      }                                                                                           \
      return FrameResult::error(setup.status);                                                    \
    }                                                                                             \
                                                                                                  \
    if (tracer_ != nullptr) [[unlikely]] {                                                        \
      tracer_->trace_post_execution(frame, ExecutionStatus::Success, gas_before - gas);           \
    }                                                                                             \
                                                                                                  \
    frame.output_offset = setup.ret_offset;                                                       \
    frame.output_size = static_cast<uint32_t>(std::min(setup.ret_size, uint64_t{UINT32_MAX}));    \
                                                                                                  \
    const auto callee_code = state_context_.code.get_code(setup.target);                          \
                                                                                                  \
    pending_frame_ = frame_pool_->rent_frame();                                                   \
    pending_frame_->pc = 0;                                                                       \
    pending_frame_->gas = setup.child_gas;                                                        \
    pending_frame_->stack = stack_pool_->rent();                                                  \
    pending_frame_->memory = memory_pool_->rent();                                                \
    pending_frame_->code_ptr = callee_code.data();                                                \
    pending_frame_->code_size = callee_code.size();                                               \
    pending_frame_->output_offset = 0;                                                            \
    pending_frame_->output_size = 0;                                                              \
    pending_frame_->depth = frame.depth + 1;                                                      \
    pending_frame_->exec_type = exec_type_val;                                                    \
    pending_frame_->is_static = is_static_expr;                                                   \
    pending_frame_->caller = caller_expr;                                                         \
    pending_frame_->address = address_expr;                                                       \
    pending_frame_->value = value_expr;                                                           \
                                                                                                  \
    if (setup.args_size > 0) {                                                                    \
      pending_frame_->input_ptr =                                                                 \
          memory.read_span_unsafe(setup.args_offset, setup.args_size).data();                     \
      pending_frame_->input_size = setup.args_size;                                               \
    } else {                                                                                      \
      pending_frame_->input_ptr = nullptr;                                                        \
      pending_frame_->input_size = 0;                                                             \
    }                                                                                             \
                                                                                                  \
    return FrameResult::call();                                                                   \
  }

  // CALL: execute code at target address, writes to target's storage
  CALL_OPCODE_HANDLER(op_call,
                      opcodes::prepare_call(stack, gas, memory, state_context_.accounts,
                                            frame.is_static, frame.depth, schedule_->memory_access),
                      ExecutionType::Call, frame.is_static, frame.address, setup.target,
                      setup.value)

  // STATICCALL: read-only call, forces static context
  CALL_OPCODE_HANDLER(op_staticcall,
                      opcodes::prepare_staticcall(stack, gas, memory, state_context_.accounts,
                                                  frame.depth, schedule_->memory_access),
                      ExecutionType::StaticCall, true, frame.address, setup.target,
                      types::Uint256::zero())

  // DELEGATECALL: execute code in caller's context (preserves msg.sender and msg.value)
  CALL_OPCODE_HANDLER(op_delegatecall,
                      opcodes::prepare_delegatecall(stack, gas, memory, state_context_.accounts,
                                                    frame.depth, schedule_->memory_access),
                      ExecutionType::DelegateCall, frame.is_static, frame.caller, frame.address,
                      frame.value)

  // CALLCODE: deprecated - like CALL but runs at caller's address
  CALL_OPCODE_HANDLER(
      op_callcode,
      opcodes::prepare_callcode(stack, gas, memory, state_context_.accounts, frame.is_static,
                                frame.depth, schedule_->memory_access),
      ExecutionType::CallCode, frame.is_static, frame.address, frame.address, setup.value)

#undef CALL_OPCODE_HANDLER

  // RETURN opcode - halt execution and return data
op_return: {
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_pre_execution(frame);
  }
  return opcodes::return_op(stack, gas, schedule_->memory_access, memory);
}

  // REVERT opcode - halt execution, revert state, and return data
op_revert: {
  if (tracer_ != nullptr) [[unlikely]] {
    tracer_->trace_pre_execution(frame);
  }
  return opcodes::revert_op(stack, gas, schedule_->memory_access, memory);
}

#undef OPCODE_HANDLER
#undef DISPATCH_NEXT

op_invalid:
  return FrameResult::error(ExecutionStatus::InvalidOpcode);
}

#pragma clang diagnostic pop
// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-pro-bounds-pointer-arithmetic,readability-function-size)

}  // namespace div0::evm
