#ifndef DIV0_EVM_EXECUTION_ENVIRONMENT_H
#define DIV0_EVM_EXECUTION_ENVIRONMENT_H

#include "div0/evm/block_context.h"
#include "div0/evm/call_params.h"
#include "div0/evm/tx_context.h"

namespace div0::evm {

/**
 * @brief Unified execution context for EVM::execute().
 *
 * Combines all context needed to execute a transaction:
 * - Block context (shared across all txs in a block, via pointer)
 * - Transaction context (per-tx: origin, gas_price, blob_hashes)
 * - Call parameters (initial call: code, input, caller, address, value, gas)
 *
 * Usage:
 *   BlockContext block_ctx = ...;  // Create once per block
 *
 *   for (auto& tx : block.transactions) {
 *       ExecutionEnvironment env{
 *           .block = &block_ctx,   // Shared pointer
 *           .tx = { .origin = ..., .gas_price = ... },
 *           .call = { .code = ..., .caller = ..., ... }
 *       };
 *       auto result = evm.execute(env);
 *   }
 */
struct ExecutionEnvironment {
  const BlockContext* block{nullptr};  // Shared across txs (non-owning)
  TxContext tx;                        // Per-transaction
  CallParams call;                     // Initial call parameters
};

}  // namespace div0::evm

#endif  // DIV0_EVM_EXECUTION_ENVIRONMENT_H
