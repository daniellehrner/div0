// output.cpp - t8n output generation

#include "t8n/output.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "div0/crypto/keccak256.h"
#include "div0/ethereum/account.h"
#include "div0/ethereum/receipt.h"
#include "div0/ethereum/roots.h"
#include "div0/ethereum/transaction/hex.h"
#include "div0/ethereum/transaction/rlp.h"
#include "div0/trie/mpt.h"

namespace div0::cli {

using ethereum::hex::encode_address;
using ethereum::hex::encode_bytes;
using ethereum::hex::encode_hash;
using ethereum::hex::encode_uint256;
using ethereum::hex::encode_uint64;

namespace {

/// Convert StorageSlot/StorageValue map to Uint256 map for compute_storage_root
std::map<types::Uint256, types::Uint256> convert_storage(
    const std::map<ethereum::StorageSlot, ethereum::StorageValue>& storage) {
  std::map<types::Uint256, types::Uint256> result;
  for (const auto& [slot, value] : storage) {
    if (!value.get().is_zero()) {
      result[slot.get()] = value.get();
    }
  }
  return result;
}

// =============================================================================
// JSON SERIALIZATION HELPERS
// =============================================================================

// NOLINTBEGIN(modernize-raw-string-literal) - JSON serialization requires escaped quotes

/// Serialize a single receipt to JSON
void serialize_receipt(std::ostream& ss, const ethereum::Receipt& receipt,
                       const std::string& indent) {
  ss << indent << "{\n";
  ss << indent << "  \"type\": \"" << encode_uint64(static_cast<uint64_t>(receipt.tx_type))
     << "\",\n";
  ss << indent << "  \"status\": \"" << encode_uint64(receipt.status) << "\",\n";
  ss << indent << "  \"cumulativeGasUsed\": \"" << encode_uint64(receipt.cumulative_gas_used)
     << "\",\n";
  ss << indent << "  \"logsBloom\": \"" << encode_bytes(receipt.bloom.span()) << "\",\n";
  ss << indent << "  \"logs\": []";  // TODO: serialize logs if needed
  ss << "\n" << indent << "}";
}

/// Serialize receipts array to JSON
void serialize_receipts(std::ostream& ss, const std::vector<ethereum::Receipt>& receipts,
                        const std::string& indent) {
  ss << indent << "\"receipts\": [\n";
  for (size_t i = 0; i < receipts.size(); ++i) {
    serialize_receipt(ss, receipts[i], indent + "  ");
    if (i + 1 < receipts.size()) {
      ss << ",";
    }
    ss << "\n";
  }
  ss << indent << "]";
}

/// Serialize rejected transactions array to JSON
void serialize_rejected(std::ostream& ss, const std::vector<RejectedTx>& rejected,
                        const std::string& indent) {
  if (rejected.empty()) {
    return;
  }
  ss << ",\n" << indent << "\"rejected\": [\n";
  for (size_t i = 0; i < rejected.size(); ++i) {
    const auto& rej = rejected[i];
    ss << indent << "  {\"index\": " << rej.index << ", \"error\": \"" << rej.error << "\"}";
    if (i + 1 < rejected.size()) {
      ss << ",";
    }
    ss << "\n";
  }
  ss << indent << "]";
}

/// Serialize result fields to JSON (core fields without outer braces)
void serialize_result_fields(std::ostream& ss, const T8nResult& result, const std::string& indent) {
  ss << indent << "\"stateRoot\": \"" << encode_hash(result.state_root) << "\",\n";
  ss << indent << "\"txRoot\": \"" << encode_hash(result.tx_root) << "\",\n";
  ss << indent << "\"receiptsRoot\": \"" << encode_hash(result.receipts_root) << "\",\n";
  ss << indent << "\"logsHash\": \"" << encode_hash(result.logs_hash) << "\",\n";
  ss << indent << "\"logsBloom\": \"" << encode_bytes(result.logs_bloom.span()) << "\",\n";
  ss << indent << "\"gasUsed\": \"" << encode_uint64(result.gas_used) << "\"";

  if (result.blob_gas_used > 0) {
    ss << ",\n" << indent << "\"blobGasUsed\": \"" << encode_uint64(result.blob_gas_used) << "\"";
  }

  // Additional fields for blockchain tests
  if (result.current_difficulty) {
    ss << ",\n"
       << indent << "\"currentDifficulty\": \"" << encode_uint256(*result.current_difficulty)
       << "\"";
  }
  if (result.current_base_fee) {
    ss << ",\n"
       << indent << "\"currentBaseFee\": \"" << encode_uint256(*result.current_base_fee) << "\"";
  }
  if (result.withdrawals_root) {
    ss << ",\n"
       << indent << "\"withdrawalsRoot\": \"" << encode_hash(*result.withdrawals_root) << "\"";
  }
  if (result.current_excess_blob_gas) {
    ss << ",\n"
       << indent << "\"currentExcessBlobGas\": \"" << encode_uint64(*result.current_excess_blob_gas)
       << "\"";
  }

  ss << ",\n";
  serialize_receipts(ss, result.receipts, indent);
  serialize_rejected(ss, result.rejected, indent);
}

/// Serialize a single account to JSON
void serialize_account(std::ostream& ss, const types::Address& address,
                       const state::AccountData& data, const T8nState& state,
                       const std::map<ethereum::StorageSlot, ethereum::StorageValue>* storage,
                       const std::string& indent) {
  ss << indent << "\"" << encode_address(address) << "\": {\n";
  ss << indent << "  \"balance\": \"" << encode_uint256(data.balance) << "\"";

  if (data.nonce > 0) {
    ss << ",\n" << indent << "  \"nonce\": \"" << encode_uint64(data.nonce) << "\"";
  }

  const auto code = state.code().get_code(address);
  if (!code.empty()) {
    ss << ",\n" << indent << "  \"code\": \"" << encode_bytes(code) << "\"";
  }

  if (storage != nullptr && !storage->empty()) {
    ss << ",\n" << indent << "  \"storage\": {\n";
    size_t j = 0;
    for (const auto& [slot, value] : *storage) {
      ss << indent << "    \"" << encode_uint256(slot.get()) << "\": \""
         << encode_uint256(value.get()) << "\"";
      if (j + 1 < storage->size()) {
        ss << ",";
      }
      ss << "\n";
      ++j;
    }
    ss << indent << "  }";
  }

  ss << "\n" << indent << "}";
}

/// Serialize alloc fields to JSON (accounts without outer braces)
void serialize_alloc_fields(std::ostream& ss, const T8nState& state, const std::string& indent) {
  const auto& accounts = state.accounts().accounts();
  const auto& storage_map = state.storage().storage();

  size_t i = 0;
  for (const auto& [address, data] : accounts) {
    auto storage_it = storage_map.find(address);
    const std::map<ethereum::StorageSlot, ethereum::StorageValue>* storage =
        (storage_it != storage_map.end()) ? &storage_it->second : nullptr;

    serialize_account(ss, address, data, state, storage, indent);

    if (i + 1 < accounts.size()) {
      ss << ",";
    }
    ss << "\n";
    ++i;
  }
}

/// Encode transactions to RLP body
types::Bytes encode_tx_body(const std::vector<ethereum::Transaction>& txs) {
  types::Bytes rlp_body;
  for (const auto& tx : txs) {
    auto tx_rlp = ethereum::rlp_encode(tx);
    rlp_body.insert(rlp_body.end(), tx_rlp.begin(), tx_rlp.end());
  }
  return rlp_body;
}

// NOLINTEND(modernize-raw-string-literal)

}  // namespace

T8nResult compute_result(const T8nState& state, const ExecutionOutput& exec_output) {
  T8nResult result;
  result.receipts = exec_output.receipts;
  result.rejected = exec_output.rejected;
  result.gas_used = exec_output.gas_used;
  result.blob_gas_used = exec_output.blob_gas_used;

  // Compute logs bloom from all receipts
  result.logs_bloom.clear();
  for (const auto& receipt : result.receipts) {
    result.logs_bloom |= receipt.bloom;
  }

  // Compute logs hash: keccak256(rlp(logs))
  std::vector<evm::Log> all_logs;
  for (const auto& receipt : result.receipts) {
    for (const auto& log : receipt.logs) {
      all_logs.push_back(log);
    }
  }
  auto encoded_logs = ethereum::rlp_encode_logs(all_logs);
  result.logs_hash = crypto::keccak256(encoded_logs);

  // Compute transactions root
  std::vector<types::Bytes> tx_rlps;
  tx_rlps.reserve(exec_output.executed_txs.size());
  for (const auto& tx : exec_output.executed_txs) {
    tx_rlps.push_back(ethereum::rlp_encode(tx));
  }
  result.tx_root = ethereum::compute_transactions_root(tx_rlps);

  // Compute receipts root
  std::vector<types::Bytes> receipt_rlps;
  receipt_rlps.reserve(result.receipts.size());
  for (const auto& receipt : result.receipts) {
    receipt_rlps.push_back(ethereum::rlp_encode(receipt));
  }
  result.receipts_root = ethereum::compute_receipts_root(receipt_rlps);

  // Compute state root from accounts with storage roots
  const auto& storage_map = state.storage().storage();
  std::map<types::Address, ethereum::AccountState> account_states;
  for (const auto& [address, data] : state.accounts().accounts()) {
    ethereum::AccountState acct_state;
    acct_state.nonce = data.nonce;
    acct_state.balance = data.balance;
    acct_state.code_hash = data.code_hash;

    auto storage_it = storage_map.find(address);
    if (storage_it != storage_map.end()) {
      auto converted = convert_storage(storage_it->second);
      acct_state.storage_root = ethereum::compute_storage_root(converted);
    } else {
      acct_state.storage_root = trie::MerklePatriciaTrie::EMPTY_ROOT;
    }

    account_states[address] = acct_state;
  }
  result.state_root = ethereum::compute_state_root(account_states);

  return result;
}

// NOLINTBEGIN(modernize-raw-string-literal) - JSON serialization requires escaped quotes

std::string serialize_result_json(const T8nResult& result) {
  std::ostringstream ss;
  ss << "{\n";
  serialize_result_fields(ss, result, "  ");
  ss << "\n}\n";
  return ss.str();
}

std::string serialize_alloc_json(const T8nState& state) {
  std::ostringstream ss;
  ss << "{\n";
  serialize_alloc_fields(ss, state, "  ");
  ss << "}\n";
  return ss.str();
}

std::string serialize_combined_output(const T8nResult& result, const T8nState& state,
                                      const std::vector<ethereum::Transaction>& txs) {
  std::ostringstream ss;
  ss << "{\n";

  // Result block
  ss << "  \"result\": {\n";
  serialize_result_fields(ss, result, "    ");
  ss << "\n  },\n";

  // Alloc block
  ss << "  \"alloc\": {\n";
  serialize_alloc_fields(ss, state, "    ");
  ss << "  },\n";

  // Body (RLP-encoded transactions)
  ss << "  \"body\": \"" << encode_bytes(encode_tx_body(txs)) << "\"\n";

  ss << "}\n";
  return ss.str();
}

// NOLINTEND(modernize-raw-string-literal)

int write_outputs(const std::string& basedir, const std::string& result_file,
                  const std::string& alloc_file, const std::string& body_file,
                  const T8nResult& result, const T8nState& state,
                  const std::vector<ethereum::Transaction>& executed_txs) {
  // When all outputs are stdout, write combined JSON
  if (result_file == "stdout" && alloc_file == "stdout" &&
      (body_file.empty() || body_file == "stdout")) {
    std::cout << serialize_combined_output(result, state, executed_txs);
    return 0;
  }

  auto write_file = [&basedir](const std::string& filename, const std::string& content) -> bool {
    if (filename == "stdout") {
      std::cout << content;
      return true;
    }

    std::string path = filename;
    if (!basedir.empty()) {
      path = basedir + "/" + filename;
    }

    std::ofstream file(path);
    if (!file) {
      std::cerr << "Error: Cannot write to " << path << "\n";
      return false;
    }
    file << content;
    return true;
  };

  if (!result_file.empty()) {
    if (!write_file(result_file, serialize_result_json(result))) {
      return 11;  // IoError
    }
  }

  if (!alloc_file.empty()) {
    if (!write_file(alloc_file, serialize_alloc_json(state))) {
      return 11;  // IoError
    }
  }

  if (!body_file.empty()) {
    if (!write_file(body_file, encode_bytes(encode_tx_body(executed_txs)) + "\n")) {
      return 11;  // IoError
    }
  }

  return 0;
}

}  // namespace div0::cli
