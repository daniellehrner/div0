// input.h - t8n input parsing

#ifndef DIV0_CLI_T8N_INPUT_H
#define DIV0_CLI_T8N_INPUT_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "div0/ethereum/transaction/transaction.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::cli {

// =============================================================================
// ALLOC TYPES
// =============================================================================

/// Pre-state account from alloc.json
struct AllocAccount {
  types::Uint256 balance;
  uint64_t nonce{0};
  types::Bytes code;
  std::map<types::Uint256, types::Uint256> storage;  // slot -> value
};

// =============================================================================
// ENV TYPES
// =============================================================================

/// Withdrawal entry (EIP-4895, Shanghai+)
struct Withdrawal {
  uint64_t index{0};
  uint64_t validator_index{0};
  types::Address address;
  uint64_t amount{0};  // In Gwei
};

/// Ommer (uncle) block header
struct Ommer {
  types::Address coinbase;
  uint64_t delta{0};  // Difference from current block number
};

/// Block environment from env.json
// NOLINTBEGIN(clang-analyzer-optin.performance.Padding)
struct EnvInput {
  types::Address coinbase;
  uint64_t gas_limit{0};
  uint64_t number{0};
  uint64_t timestamp{0};

  // Optional fields (fork-dependent)
  std::optional<types::Uint256> difficulty;       // Pre-merge
  std::optional<types::Uint256> prev_randao;      // Post-merge
  std::optional<types::Uint256> base_fee;         // EIP-1559+
  std::optional<uint64_t> excess_blob_gas;        // EIP-4844+
  std::optional<types::Hash> parent_beacon_root;  // EIP-4788+

  // For base fee calculation if base_fee not provided
  std::optional<types::Uint256> parent_base_fee;
  std::optional<uint64_t> parent_gas_used;
  std::optional<uint64_t> parent_gas_limit;

  // Block hash lookup for BLOCKHASH opcode
  std::map<uint64_t, types::Hash> block_hashes;

  // Shanghai+ withdrawals
  std::vector<Withdrawal> withdrawals;

  // Uncle/ommer headers for reward calculation
  std::vector<Ommer> ommers;
};
// NOLINTEND(clang-analyzer-optin.performance.Padding)

// =============================================================================
// PARSED INPUT DATA
// =============================================================================

/// Complete parsed input data
struct InputData {
  std::map<types::Address, AllocAccount> alloc;
  EnvInput env;
  std::vector<ethereum::Transaction> txs;
};

// =============================================================================
// ERROR TYPES
// =============================================================================

/// Parse error with exit code
struct ParseError {
  std::string message;
  int exit_code;
};

// =============================================================================
// PARSING FUNCTIONS
// =============================================================================

/// Parse alloc.json file or stdin
/// @param path File path or "stdin" for standard input
/// @return Parsed alloc map or error
[[nodiscard]] std::variant<std::map<types::Address, AllocAccount>, ParseError> parse_alloc(
    const std::string& path);

/// Parse env.json file or stdin
/// @param path File path or "stdin" for standard input
/// @return Parsed env or error
[[nodiscard]] std::variant<EnvInput, ParseError> parse_env(const std::string& path);

/// Parse txs.json file or stdin (can also parse RLP file)
/// @param path File path or "stdin" for standard input
/// @return Parsed transactions or error
[[nodiscard]] std::variant<std::vector<ethereum::Transaction>, ParseError> parse_txs(
    const std::string& path);

/// Parse all inputs
/// @param alloc_path Path to alloc.json or "stdin"
/// @param env_path Path to env.json or "stdin"
/// @param txs_path Path to txs.json or "stdin"
/// @return Complete input data or error
[[nodiscard]] std::variant<InputData, ParseError> parse_inputs(const std::string& alloc_path,
                                                               const std::string& env_path,
                                                               const std::string& txs_path);

}  // namespace div0::cli

#endif  // DIV0_CLI_T8N_INPUT_H
