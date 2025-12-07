// executor.h - Transaction execution for t8n

#ifndef DIV0_CLI_T8N_EXECUTOR_H
#define DIV0_CLI_T8N_EXECUTOR_H

#include <cstdint>
#include <string>
#include <vector>

#include "div0/crypto/secp256k1.h"
#include "div0/ethereum/receipt.h"
#include "div0/ethereum/transaction/transaction.h"
#include "div0/evm/forks.h"
#include "div0/evm/tracer_config.h"
#include "div0/types/hash.h"
#include "t8n/input.h"
#include "t8n/state_builder.h"

namespace div0::cli {

// =============================================================================
// TRACE CONFIG
// =============================================================================

/// Trace configuration for transaction execution
struct TraceConfig {
  bool enabled = false;
  std::string output_dir = ".";  // Directory for trace files
  evm::TracerConfig tracer_config{};
};

// =============================================================================
// RESULT TYPES
// =============================================================================

/// Rejected transaction info
struct RejectedTx {
  size_t index;
  std::string error;
};

/// Result of transaction execution
struct ExecutionOutput {
  std::vector<ethereum::Transaction> executed_txs;
  std::vector<ethereum::Receipt> receipts;
  std::vector<types::Hash> tx_hashes;  // Transaction hashes (same order as receipts)
  std::vector<RejectedTx> rejected;
  uint64_t gas_used{0};
  uint64_t blob_gas_used{0};
};

// =============================================================================
// EXECUTION FUNCTION
// =============================================================================

/// Execute all transactions against state.
///
/// This is the main t8n execution loop that:
/// 1. Iterates transactions in order
/// 2. Recovers sender addresses
/// 3. Validates each transaction (nonce, balance, gas limits, etc.)
/// 4. Executes valid transactions through EVM
/// 5. Builds receipts and tracks rejected transactions
///
/// @param state T8nState to modify
/// @param txs Transactions to execute (in order)
/// @param env Block environment
/// @param fork EVM fork rules
/// @param chain_id Chain ID for transaction validation
/// @param secp_ctx Secp256k1 context for signature recovery
/// @param trace_config Optional trace configuration
/// @return Execution output with receipts and rejected transactions
[[nodiscard]] ExecutionOutput execute_transactions(T8nState& state,
                                                   const std::vector<ethereum::Transaction>& txs,
                                                   const EnvInput& env, evm::Fork fork,
                                                   uint64_t chain_id,
                                                   crypto::Secp256k1Context& secp_ctx,
                                                   const TraceConfig& trace_config = {});

}  // namespace div0::cli

#endif  // DIV0_CLI_T8N_EXECUTOR_H
