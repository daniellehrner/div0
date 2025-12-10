// output.h - t8n output generation

#ifndef DIV0_CLI_T8N_OUTPUT_H
#define DIV0_CLI_T8N_OUTPUT_H

#include <optional>
#include <string>
#include <vector>

#include "div0/ethereum/bloom.h"
#include "div0/ethereum/receipt.h"
#include "div0/ethereum/transaction/transaction.h"
#include "div0/types/hash.h"
#include "t8n/executor.h"
#include "t8n/state_builder.h"

namespace div0::cli {

// =============================================================================
// RESULT TYPES
// =============================================================================

/// Complete t8n execution result
struct T8nResult {
  types::Hash state_root;
  types::Hash tx_root;
  types::Hash receipts_root;
  types::Hash logs_hash;  // keccak256(rlp(logs))
  ethereum::Bloom logs_bloom;
  std::vector<ethereum::Receipt> receipts;
  std::vector<types::Hash> tx_hashes;  // Transaction hashes (same order as receipts)
  std::vector<RejectedTx> rejected;
  uint64_t gas_used{0};
  uint64_t blob_gas_used{0};

  // Additional fields for blockchain tests
  std::optional<types::Uint256> current_difficulty;  // Pre-merge
  std::optional<types::Uint256> current_base_fee;    // EIP-1559+
  std::optional<types::Hash> withdrawals_root;       // Shanghai+
  std::optional<uint64_t> current_excess_blob_gas;   // EIP-4844+
};

// =============================================================================
// OUTPUT FUNCTIONS
// =============================================================================

/// Compute result from execution output and final state
/// @param state Final state after execution
/// @param exec_output Execution output with receipts and rejected txs
/// @return Complete t8n result with computed roots
[[nodiscard]] T8nResult compute_result(const T8nState& state, const ExecutionOutput& exec_output);

/// Serialize result to JSON (go-ethereum compatible format)
/// @param result T8n result to serialize
/// @return JSON string
[[nodiscard]] std::string serialize_result_json(const T8nResult& result);

/// Serialize post-state alloc to JSON
/// @param state Final state to serialize
/// @return JSON string with account balances, nonces, code, storage
[[nodiscard]] std::string serialize_alloc_json(const T8nState& state);

/// Write outputs to files or stdout
/// @param basedir Base directory for output files (empty for cwd)
/// @param result_file Output filename for result ("stdout" for stdout, empty to skip)
/// @param alloc_file Output filename for alloc ("stdout" for stdout, empty to skip)
/// @param body_file Output filename for body ("stdout" for stdout, empty to skip)
/// @param result T8n result to write
/// @param state Final state for alloc output
/// @param executed_txs Executed transactions for body output
/// @return 0 on success, non-zero exit code on failure
[[nodiscard]] int write_outputs(const std::string& basedir, const std::string& result_file,
                                const std::string& alloc_file, const std::string& body_file,
                                const T8nResult& result, const T8nState& state,
                                const std::vector<ethereum::Transaction>& executed_txs);

}  // namespace div0::cli

#endif  // DIV0_CLI_T8N_OUTPUT_H
