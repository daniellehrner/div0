#include <cassert>
#include <cstdint>
#include <span>

#include "div0/db/memory_database.h"
#include "div0/evm/block_context.h"
#include "div0/evm/evm.h"
#include "div0/evm/execution_environment.h"
#include "div0/evm/forks.h"
#include "div0/state/memory_account_provider.h"
#include "div0/state/memory_code_provider.h"
#include "div0/state/memory_storage_provider.h"
#include "div0/state/state_context.h"

// libFuzzer entry point - called repeatedly with mutated input data
// NOLINTNEXTLINE(readability-identifier-naming) - function name required by libFuzzer API
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Fuzzer provides random bytes - we treat them as EVM bytecode
  const std::span code(data, size);

  // Fresh state each run for deterministic behavior
  div0::db::MemoryDatabase database;
  div0::state::MemoryStorageProvider storage(database);
  div0::state::MemoryAccountProvider accounts;
  div0::state::MemoryCodeProvider code_provider;
  div0::state::StateContext state_context(storage, accounts, code_provider);

  div0::evm::EVM evm(state_context, div0::evm::Fork::Shanghai);

  // Set up execution environment
  constexpr div0::evm::BlockContext BLOCK_CTX{};
  const div0::evm::ExecutionEnvironment env{.block = &BLOCK_CTX,
                                            .tx = {},
                                            .call = {.value = div0::types::Uint256::zero(),
                                                     .gas = 1000000,
                                                     .code = code,
                                                     .input = {},
                                                     .caller = div0::types::Address::zero(),
                                                     .address = div0::types::Address::zero(),
                                                     .is_static = false}};

  // Execute with bounded gas (prevents infinite loops)
  // ASan catches: buffer overflows, use-after-free
  // UBSan catches: shift overflow, signed overflow
  const auto result = evm.execute(env);

  // Sanity check: result must be a known status value
  assert(result.status == div0::evm::ExecutionStatus::Success ||
         result.status == div0::evm::ExecutionStatus::Revert ||
         result.status == div0::evm::ExecutionStatus::OutOfGas ||
         result.status == div0::evm::ExecutionStatus::InvalidOpcode ||
         result.status == div0::evm::ExecutionStatus::StackOverflow ||
         result.status == div0::evm::ExecutionStatus::StackUnderflow ||
         result.status == div0::evm::ExecutionStatus::InvalidJump ||
         result.status == div0::evm::ExecutionStatus::WriteProtection ||
         result.status == div0::evm::ExecutionStatus::CallDepthExceeded);

  return 0;  // Always accept input into corpus
}
