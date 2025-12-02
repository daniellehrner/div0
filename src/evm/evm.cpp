#include "div0/evm/evm.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <stack>

#include "div0/evm/gas/dynamic_costs.h"
#include "gas/static_costs.h"
#include "opcodes.h"
#include "opcodes/arithmetic.h"
#include "opcodes/call.h"
#include "opcodes/comparison.h"
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
    const uint64_t gas_used = initial_gas - frame->gas;
    auto return_data = (result.action == FrameResult::Action::Return)
                           ? extract_return_data(*frame, result)
                           : std::vector<uint8_t>{};
    release_frame_resources();
    return make_success_result(gas_used, std::move(return_data));
  };

  // Handle top-level revert
  auto handle_top_level_revert = [&](const FrameResult& result) -> ExecutionResult {
    const uint64_t gas_used = initial_gas - frame->gas;
    auto revert_data = extract_return_data(*frame, result);
    release_frame_resources();
    return make_revert_result(gas_used, std::move(revert_data));
  };

  // Handle top-level error
  auto handle_top_level_error = [&](const FrameResult& result) -> ExecutionResult {
    const uint64_t gas_used = initial_gas - frame->gas;
    release_frame_resources();
    return make_error_result(result.error_status, gas_used);
  };

  // Handle child frame completion (success or revert) - returns to parent
  auto handle_child_completion = [&](const FrameResult& result) {
    const CallFrame* child = frame;
    frame = frame_stack.top();
    frame_stack.pop();
    frame->gas += child->gas;  // Refund unused gas
    handle_child_return(*frame, *child, result);
    release_frame_resources();
  };

  // Handle child frame error - returns to parent with failure
  auto handle_child_error = [&]() {
    frame = frame_stack.top();
    frame_stack.pop();
    // Child consumed all its gas on error (no refund)
    return_data_.clear();
    if (frame->stack != nullptr) {
      (void)frame->stack->push(types::Uint256::zero());
    }
    release_frame_resources();
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
      dispatch_shanghai[op(Opcode::Stop)] = &&op_stop;
      dispatch_shanghai[op(Opcode::Add)] = &&op_add;
      dispatch_shanghai[op(Opcode::Mul)] = &&op_mul;
      dispatch_shanghai[op(Opcode::Sub)] = &&op_sub;
      dispatch_shanghai[op(Opcode::Div)] = &&op_div;
      dispatch_shanghai[op(Opcode::SDiv)] = &&op_sdiv;
      dispatch_shanghai[op(Opcode::Mod)] = &&op_mod;
      dispatch_shanghai[op(Opcode::SMod)] = &&op_smod;
      dispatch_shanghai[op(Opcode::AddMod)] = &&op_addmod;
      dispatch_shanghai[op(Opcode::MulMod)] = &&op_mulmod;
      dispatch_shanghai[op(Opcode::Exp)] = &&op_exp;
      dispatch_shanghai[op(Opcode::SignExtend)] = &&op_signextend;
      dispatch_shanghai[op(Opcode::Lt)] = &&op_lt;
      dispatch_shanghai[op(Opcode::Gt)] = &&op_gt;
      dispatch_shanghai[op(Opcode::Eq)] = &&op_eq;
      dispatch_shanghai[op(Opcode::IsZero)] = &&op_is_zero;
      dispatch_shanghai[op(Opcode::Sha3)] = &&op_sha3;
      dispatch_shanghai[op(Opcode::MLoad)] = &&op_mload;
      dispatch_shanghai[op(Opcode::MStore)] = &&op_mstore;
      dispatch_shanghai[op(Opcode::MStore8)] = &&op_mstore8;
      dispatch_shanghai[op(Opcode::SLoad)] = &&op_sload;
      dispatch_shanghai[op(Opcode::SStore)] = &&op_sstore;
      dispatch_shanghai[op(Opcode::MSize)] = &&op_msize;
      dispatch_shanghai[op(Opcode::Push0)] = &&op_push0;
      dispatch_shanghai[op(Opcode::Push1)] = &&op_push1;
      dispatch_shanghai[op(Opcode::Push2)] = &&op_push2;
      dispatch_shanghai[op(Opcode::Push3)] = &&op_push3;
      dispatch_shanghai[op(Opcode::Push4)] = &&op_push4;
      dispatch_shanghai[op(Opcode::Push5)] = &&op_push5;
      dispatch_shanghai[op(Opcode::Push6)] = &&op_push6;
      dispatch_shanghai[op(Opcode::Push7)] = &&op_push7;
      dispatch_shanghai[op(Opcode::Push8)] = &&op_push8;
      dispatch_shanghai[op(Opcode::Push9)] = &&op_push9;
      dispatch_shanghai[op(Opcode::Push10)] = &&op_push10;
      dispatch_shanghai[op(Opcode::Push11)] = &&op_push11;
      dispatch_shanghai[op(Opcode::Push12)] = &&op_push12;
      dispatch_shanghai[op(Opcode::Push13)] = &&op_push13;
      dispatch_shanghai[op(Opcode::Push14)] = &&op_push14;
      dispatch_shanghai[op(Opcode::Push15)] = &&op_push15;
      dispatch_shanghai[op(Opcode::Push16)] = &&op_push16;
      dispatch_shanghai[op(Opcode::Push17)] = &&op_push17;
      dispatch_shanghai[op(Opcode::Push18)] = &&op_push18;
      dispatch_shanghai[op(Opcode::Push19)] = &&op_push19;
      dispatch_shanghai[op(Opcode::Push20)] = &&op_push20;
      dispatch_shanghai[op(Opcode::Push21)] = &&op_push21;
      dispatch_shanghai[op(Opcode::Push22)] = &&op_push22;
      dispatch_shanghai[op(Opcode::Push23)] = &&op_push23;
      dispatch_shanghai[op(Opcode::Push24)] = &&op_push24;
      dispatch_shanghai[op(Opcode::Push25)] = &&op_push25;
      dispatch_shanghai[op(Opcode::Push26)] = &&op_push26;
      dispatch_shanghai[op(Opcode::Push27)] = &&op_push27;
      dispatch_shanghai[op(Opcode::Push28)] = &&op_push28;
      dispatch_shanghai[op(Opcode::Push29)] = &&op_push29;
      dispatch_shanghai[op(Opcode::Push30)] = &&op_push30;
      dispatch_shanghai[op(Opcode::Push31)] = &&op_push31;
      dispatch_shanghai[op(Opcode::Push32)] = &&op_push32;
      dispatch_shanghai[op(Opcode::Dup1)] = &&op_dup1;
      dispatch_shanghai[op(Opcode::Dup2)] = &&op_dup2;
      dispatch_shanghai[op(Opcode::Dup3)] = &&op_dup3;
      dispatch_shanghai[op(Opcode::Dup4)] = &&op_dup4;
      dispatch_shanghai[op(Opcode::Dup5)] = &&op_dup5;
      dispatch_shanghai[op(Opcode::Dup6)] = &&op_dup6;
      dispatch_shanghai[op(Opcode::Dup7)] = &&op_dup7;
      dispatch_shanghai[op(Opcode::Dup8)] = &&op_dup8;
      dispatch_shanghai[op(Opcode::Dup9)] = &&op_dup9;
      dispatch_shanghai[op(Opcode::Dup10)] = &&op_dup10;
      dispatch_shanghai[op(Opcode::Dup11)] = &&op_dup11;
      dispatch_shanghai[op(Opcode::Dup12)] = &&op_dup12;
      dispatch_shanghai[op(Opcode::Dup13)] = &&op_dup13;
      dispatch_shanghai[op(Opcode::Dup14)] = &&op_dup14;
      dispatch_shanghai[op(Opcode::Dup15)] = &&op_dup15;
      dispatch_shanghai[op(Opcode::Dup16)] = &&op_dup16;
      dispatch_shanghai[op(Opcode::Swap1)] = &&op_swap1;
      dispatch_shanghai[op(Opcode::Swap2)] = &&op_swap2;
      dispatch_shanghai[op(Opcode::Swap3)] = &&op_swap3;
      dispatch_shanghai[op(Opcode::Swap4)] = &&op_swap4;
      dispatch_shanghai[op(Opcode::Swap5)] = &&op_swap5;
      dispatch_shanghai[op(Opcode::Swap6)] = &&op_swap6;
      dispatch_shanghai[op(Opcode::Swap7)] = &&op_swap7;
      dispatch_shanghai[op(Opcode::Swap8)] = &&op_swap8;
      dispatch_shanghai[op(Opcode::Swap9)] = &&op_swap9;
      dispatch_shanghai[op(Opcode::Swap10)] = &&op_swap10;
      dispatch_shanghai[op(Opcode::Swap11)] = &&op_swap11;
      dispatch_shanghai[op(Opcode::Swap12)] = &&op_swap12;
      dispatch_shanghai[op(Opcode::Swap13)] = &&op_swap13;
      dispatch_shanghai[op(Opcode::Swap14)] = &&op_swap14;
      dispatch_shanghai[op(Opcode::Swap15)] = &&op_swap15;
      dispatch_shanghai[op(Opcode::Swap16)] = &&op_swap16;
      dispatch_shanghai[op(Opcode::Call)] = &&op_call;

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

#define OPCODE_HANDLER(name, func, opcode_enum)                                            \
  op_##name : status = func(stack, gas, schedule_->static_costs[op(Opcode::opcode_enum)]); \
  if (status != ExecutionStatus::Success) [[unlikely]] {                                   \
    return FrameResult::error(status);                                                     \
  }                                                                                        \
  DISPATCH_NEXT();

  // Start execution
  DISPATCH_NEXT();

op_stop:
  return FrameResult::stop();

  // Arithmetic opcodes
  OPCODE_HANDLER(add, opcodes::add, Add)
  OPCODE_HANDLER(mul, opcodes::mul, Mul)
  OPCODE_HANDLER(sub, opcodes::sub, Sub)
  OPCODE_HANDLER(div, opcodes::div, Div)
  OPCODE_HANDLER(sdiv, opcodes::sdiv, SDiv)
  OPCODE_HANDLER(mod, opcodes::mod, Mod)
  OPCODE_HANDLER(smod, opcodes::smod, SMod)
  OPCODE_HANDLER(addmod, opcodes::addmod, AddMod)
  OPCODE_HANDLER(mulmod, opcodes::mulmod, MulMod)
  OPCODE_HANDLER(signextend, opcodes::signextend, SignExtend)

op_exp:
  status = opcodes::exp(stack, gas, schedule_->exp);
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
  DISPATCH_NEXT();

  // Comparison opcodes
  OPCODE_HANDLER(lt, opcodes::lt, Lt)
  OPCODE_HANDLER(gt, opcodes::gt, Gt)
  OPCODE_HANDLER(eq, opcodes::eq, Eq)
  OPCODE_HANDLER(is_zero, opcodes::is_zero, IsZero)

op_sha3:
  status = opcodes::keccak256(stack, gas, schedule_->static_costs[op(Opcode::Sha3)],
                              schedule_->sha3_word_cost, schedule_->memory_access, memory);
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
  DISPATCH_NEXT();

#define MEMORY_OPCODE_HANDLER(name, func, opcode_enum)                                    \
  op_##name : status = func(stack, gas, schedule_->static_costs[op(Opcode::opcode_enum)], \
                            schedule_->memory_access, memory);                            \
  if (status != ExecutionStatus::Success) [[unlikely]] {                                  \
    return FrameResult::error(status);                                                    \
  }                                                                                       \
  DISPATCH_NEXT();

  MEMORY_OPCODE_HANDLER(mload, opcodes::mload, MLoad)
  MEMORY_OPCODE_HANDLER(mstore, opcodes::mstore, MStore)
  MEMORY_OPCODE_HANDLER(mstore8, opcodes::mstore8, MStore8)

#undef MEMORY_OPCODE_HANDLER

op_msize:
  status = opcodes::msize(stack, gas, schedule_->static_costs[op(Opcode::MSize)], memory);
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
  DISPATCH_NEXT();

op_sload:
  status = opcodes::sload(frame.address, stack, gas, state_context_.storage, schedule_->sload);
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
  DISPATCH_NEXT();

op_sstore:
  if (frame.is_static) [[unlikely]] {
    return FrameResult::error(ExecutionStatus::WriteProtection);
  }
  status = opcodes::sstore(frame.address, stack, gas, state_context_.storage, schedule_->sstore);
  if (status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(status);
  }
  DISPATCH_NEXT();

  // PUSH0: push zero onto stack
  OPCODE_HANDLER(push0, opcodes::push0, Push0)

// PUSH1-32: push N bytes from bytecode onto stack
#define PUSH_N_HANDLER(n, opcode_enum)                                                      \
  op_push##n : status = opcodes::push_n<n>(                                                 \
                   stack, gas, schedule_->static_costs[op(Opcode::opcode_enum)], code, pc); \
  if (status != ExecutionStatus::Success) [[unlikely]] {                                    \
    return FrameResult::error(status);                                                      \
  }                                                                                         \
  DISPATCH_NEXT();

  PUSH_N_HANDLER(1, Push1)
  PUSH_N_HANDLER(2, Push2)
  PUSH_N_HANDLER(3, Push3)
  PUSH_N_HANDLER(4, Push4)
  PUSH_N_HANDLER(5, Push5)
  PUSH_N_HANDLER(6, Push6)
  PUSH_N_HANDLER(7, Push7)
  PUSH_N_HANDLER(8, Push8)
  PUSH_N_HANDLER(9, Push9)
  PUSH_N_HANDLER(10, Push10)
  PUSH_N_HANDLER(11, Push11)
  PUSH_N_HANDLER(12, Push12)
  PUSH_N_HANDLER(13, Push13)
  PUSH_N_HANDLER(14, Push14)
  PUSH_N_HANDLER(15, Push15)
  PUSH_N_HANDLER(16, Push16)
  PUSH_N_HANDLER(17, Push17)
  PUSH_N_HANDLER(18, Push18)
  PUSH_N_HANDLER(19, Push19)
  PUSH_N_HANDLER(20, Push20)
  PUSH_N_HANDLER(21, Push21)
  PUSH_N_HANDLER(22, Push22)
  PUSH_N_HANDLER(23, Push23)
  PUSH_N_HANDLER(24, Push24)
  PUSH_N_HANDLER(25, Push25)
  PUSH_N_HANDLER(26, Push26)
  PUSH_N_HANDLER(27, Push27)
  PUSH_N_HANDLER(28, Push28)
  PUSH_N_HANDLER(29, Push29)
  PUSH_N_HANDLER(30, Push30)
  PUSH_N_HANDLER(31, Push31)
  PUSH_N_HANDLER(32, Push32)

#undef PUSH_N_HANDLER

#define DUP_N_HANDLER(n, opcode_enum)                                                              \
  op_dup##n : status =                                                                             \
                  opcodes::dup_n(stack, gas, schedule_->static_costs[op(Opcode::opcode_enum)], n); \
  if (status != ExecutionStatus::Success) [[unlikely]] {                                           \
    return FrameResult::error(status);                                                             \
  }                                                                                                \
  DISPATCH_NEXT();

  DUP_N_HANDLER(1, Dup1)
  DUP_N_HANDLER(2, Dup2)
  DUP_N_HANDLER(3, Dup3)
  DUP_N_HANDLER(4, Dup4)
  DUP_N_HANDLER(5, Dup5)
  DUP_N_HANDLER(6, Dup6)
  DUP_N_HANDLER(7, Dup7)
  DUP_N_HANDLER(8, Dup8)
  DUP_N_HANDLER(9, Dup9)
  DUP_N_HANDLER(10, Dup10)
  DUP_N_HANDLER(11, Dup11)
  DUP_N_HANDLER(12, Dup12)
  DUP_N_HANDLER(13, Dup13)
  DUP_N_HANDLER(14, Dup14)
  DUP_N_HANDLER(15, Dup15)
  DUP_N_HANDLER(16, Dup16)

#undef DUP_N_HANDLER

#define SWAP_N_HANDLER(n, opcode_enum)                                                             \
  op_swap##n                                                                                       \
      : status = opcodes::swap_n(stack, gas, schedule_->static_costs[op(Opcode::opcode_enum)], n); \
  if (status != ExecutionStatus::Success) [[unlikely]] {                                           \
    return FrameResult::error(status);                                                             \
  }                                                                                                \
  DISPATCH_NEXT();

  SWAP_N_HANDLER(1, Swap1)
  SWAP_N_HANDLER(2, Swap2)
  SWAP_N_HANDLER(3, Swap3)
  SWAP_N_HANDLER(4, Swap4)
  SWAP_N_HANDLER(5, Swap5)
  SWAP_N_HANDLER(6, Swap6)
  SWAP_N_HANDLER(7, Swap7)
  SWAP_N_HANDLER(8, Swap8)
  SWAP_N_HANDLER(9, Swap9)
  SWAP_N_HANDLER(10, Swap10)
  SWAP_N_HANDLER(11, Swap11)
  SWAP_N_HANDLER(12, Swap12)
  SWAP_N_HANDLER(13, Swap13)
  SWAP_N_HANDLER(14, Swap14)
  SWAP_N_HANDLER(15, Swap15)
  SWAP_N_HANDLER(16, Swap16)

#undef SWAP_N_HANDLER

  // CALL opcode - creates a child call frame
op_call: {
  auto setup = opcodes::prepare_call(stack, gas, memory, state_context_.accounts, frame.is_static,
                                     frame.depth, schedule_->memory_access);

  // Call depth exceeded is not a fatal error - the EVM spec says we simply
  // fail the call (return 0) but continue execution. prepare_call() already
  // pushed 0 onto the stack, so we just continue to the next opcode.
  if (setup.status == ExecutionStatus::CallDepthExceeded) [[unlikely]] {
    DISPATCH_NEXT();
  }

  if (setup.status != ExecutionStatus::Success) [[unlikely]] {
    return FrameResult::error(setup.status);
  }

  // Store output location in parent frame (for when child returns)
  frame.output_offset = setup.ret_offset;
  frame.output_size = static_cast<uint32_t>(std::min(setup.ret_size, uint64_t{UINT32_MAX}));

  // Get callee code
  const auto callee_code = state_context_.code.get_code(setup.target);

  // Create child call frame
  pending_frame_ = frame_pool_->rent_frame();
  pending_frame_->pc = 0;
  pending_frame_->gas = setup.child_gas;
  pending_frame_->stack = stack_pool_->rent();
  pending_frame_->memory = memory_pool_->rent();
  pending_frame_->code_ptr = callee_code.data();
  pending_frame_->code_size = callee_code.size();
  pending_frame_->output_offset = 0;
  pending_frame_->output_size = 0;
  pending_frame_->depth = frame.depth + 1;
  pending_frame_->exec_type = ExecutionType::Call;
  pending_frame_->is_static = frame.is_static;  // Inherit static context
  pending_frame_->caller = frame.address;       // Caller is current contract
  pending_frame_->address = setup.target;       // Target address
  pending_frame_->value = setup.value;          // Value being sent

  // Set input data pointer into parent's memory.
  // SAFETY: This pointer remains valid because each frame has its own Memory instance.
  // The parent's memory is not modified while the child executes (parent is suspended).
  if (setup.args_size > 0) {
    pending_frame_->input_ptr = memory.read_span_unsafe(setup.args_offset, setup.args_size).data();
    pending_frame_->input_size = setup.args_size;
  } else {
    pending_frame_->input_ptr = nullptr;
    pending_frame_->input_size = 0;
  }

  // Signal to execute() that we need to run a child call
  return FrameResult::call();
}

#undef OPCODE_HANDLER
#undef DISPATCH_NEXT

op_invalid:
  return FrameResult::error(ExecutionStatus::InvalidOpcode);
}

#pragma clang diagnostic pop
// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-pro-bounds-pointer-arithmetic,readability-function-size)

}  // namespace div0::evm
