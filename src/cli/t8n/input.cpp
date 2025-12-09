// input.cpp - t8n input parsing implementation

#include "t8n/input.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "div0/ethereum/transaction/json.h"
#include "div0/utils/hex.h"
#include "exit_codes.h"

// simdjson has various warnings in its template code
// NOLINTBEGIN(clang-diagnostic-sign-conversion,clang-diagnostic-format-nonliteral)
#include <simdjson.h>
// NOLINTEND(clang-diagnostic-sign-conversion,clang-diagnostic-format-nonliteral)

namespace div0::cli {

using hex::decode_address;
using hex::decode_bytes;
using hex::decode_hash;
using hex::decode_uint256;
using hex::decode_uint64;

// =============================================================================
// HELPER UTILITIES
// =============================================================================

namespace {

/// Read content from file or stdin
std::variant<std::string, ParseError> read_input(const std::string& path) {
  if (path == "stdin") {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return ss.str();
  }

  // NOLINTNEXTLINE(misc-const-correctness) - ifstream cannot be const, reading modifies state
  std::ifstream file(path);
  if (!file) {
    return ParseError{.message = "cannot open file: " + path,
                      .exit_code = static_cast<int>(ExitCode::IoError)};
  }

  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

/// Helper to get optional string field from JSON object
std::optional<std::string_view> get_string_field(simdjson::ondemand::object& obj,
                                                 std::string_view key) {
  auto field = obj.find_field_unordered(key);
  if (field.error() != simdjson::SUCCESS) {
    return std::nullopt;
  }
  auto str = field.get_string();
  if (str.error() != simdjson::SUCCESS) {
    return std::nullopt;
  }
  return str.value();
}

/// Create a JSON parse error
ParseError json_error(const std::string& message) {
  return ParseError{.message = message, .exit_code = static_cast<int>(ExitCode::JsonError)};
}

// =============================================================================
// ALLOC PARSING
// =============================================================================

/// Parse alloc from JSON object
std::variant<std::map<types::Address, AllocAccount>, ParseError> parse_alloc_from_json(
    simdjson::ondemand::object& root) {
  std::map<types::Address, AllocAccount> result;

  for (auto field : root) {
    const std::string_view addr_hex = field.unescaped_key();

    auto addr_opt = decode_address(addr_hex);
    if (!addr_opt) {
      return json_error("invalid address in alloc: " + std::string(addr_hex));
    }

    auto account_obj = field.value().get_object();
    if (account_obj.error() != simdjson::SUCCESS) {
      return json_error("account must be an object: " + std::string(addr_hex));
    }

    AllocAccount account;

    // Balance (optional, default 0)
    if (auto balance_str = get_string_field(account_obj.value(), "balance")) {
      auto balance = decode_uint256(*balance_str);
      if (!balance) {
        return json_error("invalid balance for " + std::string(addr_hex));
      }
      account.balance = *balance;
    }

    // Nonce (optional, default 0)
    if (auto nonce_str = get_string_field(account_obj.value(), "nonce")) {
      auto nonce = decode_uint64(*nonce_str);
      if (!nonce) {
        return json_error("invalid nonce for " + std::string(addr_hex));
      }
      account.nonce = *nonce;
    }

    // Code (optional)
    if (auto code_str = get_string_field(account_obj.value(), "code")) {
      auto code = decode_bytes(*code_str);
      if (!code) {
        return json_error("invalid code for " + std::string(addr_hex));
      }
      account.code = std::move(*code);
    }

    // Storage (optional)
    auto storage_field = account_obj.value().find_field_unordered("storage");
    if (storage_field.error() == simdjson::SUCCESS) {
      auto storage_obj = storage_field.get_object();
      if (storage_obj.error() == simdjson::SUCCESS) {
        for (auto slot_field : storage_obj.value()) {
          const std::string_view slot_hex = slot_field.unescaped_key();
          auto slot = decode_uint256(slot_hex);
          if (!slot) {
            return json_error("invalid storage slot for " + std::string(addr_hex));
          }

          auto value_str = slot_field.value().get_string();
          if (value_str.error() != simdjson::SUCCESS) {
            return json_error("invalid storage value for " + std::string(addr_hex));
          }
          auto value = decode_uint256(value_str.value());
          if (!value) {
            return json_error("invalid storage value for " + std::string(addr_hex));
          }

          if (!value->is_zero()) {
            account.storage[*slot] = *value;
          }
        }
      }
    }

    result[*addr_opt] = std::move(account);
  }

  return result;
}

// =============================================================================
// ENV PARSING
// =============================================================================

/// Parse blockHashes from JSON object into map
std::optional<ParseError> parse_block_hashes(simdjson::ondemand::object& obj,
                                             std::map<uint64_t, types::Hash>& out) {
  auto field = obj.find_field_unordered("blockHashes");
  if (field.error() != simdjson::SUCCESS) {
    return std::nullopt;  // Optional field, not an error
  }

  auto hashes_obj = field.get_object();
  if (hashes_obj.error() != simdjson::SUCCESS) {
    return std::nullopt;
  }

  for (auto hash_field : hashes_obj.value()) {
    const std::string_view block_num_str = hash_field.unescaped_key();
    auto block_num = decode_uint64(block_num_str);
    if (!block_num) {
      return json_error("invalid block number in blockHashes");
    }

    auto hash_str = hash_field.value().get_string();
    if (hash_str.error() != simdjson::SUCCESS) {
      return json_error("invalid hash in blockHashes");
    }
    auto hash = decode_hash(hash_str.value());
    if (!hash) {
      return json_error("invalid hash in blockHashes");
    }

    out[*block_num] = *hash;
  }

  return std::nullopt;
}

/// Parse withdrawals array from JSON object
std::optional<ParseError> parse_withdrawals(simdjson::ondemand::object& obj,
                                            std::vector<Withdrawal>& out) {
  auto field = obj.find_field_unordered("withdrawals");
  if (field.error() != simdjson::SUCCESS) {
    return std::nullopt;
  }

  auto arr = field.get_array();
  if (arr.error() != simdjson::SUCCESS) {
    return std::nullopt;
  }

  for (auto w_val : arr.value()) {
    auto w_obj = w_val.get_object();
    if (w_obj.error() != simdjson::SUCCESS) {
      return json_error("invalid withdrawal object");
    }

    Withdrawal w;

    if (auto s = get_string_field(w_obj.value(), "index")) {
      if (auto v = decode_uint64(*s)) {
        w.index = *v;
      }
    }
    if (auto s = get_string_field(w_obj.value(), "validatorIndex")) {
      if (auto v = decode_uint64(*s)) {
        w.validator_index = *v;
      }
    }
    if (auto s = get_string_field(w_obj.value(), "address")) {
      if (auto v = decode_address(*s)) {
        w.address = *v;
      }
    }
    if (auto s = get_string_field(w_obj.value(), "amount")) {
      if (auto v = decode_uint64(*s)) {
        w.amount = *v;
      }
    }

    out.push_back(w);
  }

  return std::nullopt;
}

/// Parse ommers array from JSON object
std::optional<ParseError> parse_ommers(simdjson::ondemand::object& obj, std::vector<Ommer>& out) {
  auto field = obj.find_field_unordered("ommers");
  if (field.error() != simdjson::SUCCESS) {
    return std::nullopt;
  }

  auto arr = field.get_array();
  if (arr.error() != simdjson::SUCCESS) {
    return std::nullopt;
  }

  for (auto o_val : arr.value()) {
    auto o_obj = o_val.get_object();
    if (o_obj.error() != simdjson::SUCCESS) {
      return json_error("invalid ommer object");
    }

    Ommer o;

    if (auto s = get_string_field(o_obj.value(), "coinbase")) {
      if (auto v = decode_address(*s)) {
        o.coinbase = *v;
      }
    }
    if (auto s = get_string_field(o_obj.value(), "delta")) {
      if (auto v = decode_uint64(*s)) {
        o.delta = *v;
      }
    }

    out.push_back(o);
  }

  return std::nullopt;
}

/// Parse env from JSON object - shared by parse_env() and parse_combined_stdin()
std::variant<EnvInput, ParseError> parse_env_from_json(simdjson::ondemand::object& obj) {
  EnvInput env;

  // Required fields
  auto coinbase_str = get_string_field(obj, "currentCoinbase");
  if (!coinbase_str) {
    return json_error("env missing currentCoinbase");
  }
  auto coinbase = decode_address(*coinbase_str);
  if (!coinbase) {
    return json_error("invalid currentCoinbase");
  }
  env.coinbase = *coinbase;

  auto gas_limit_str = get_string_field(obj, "currentGasLimit");
  if (!gas_limit_str) {
    return json_error("env missing currentGasLimit");
  }
  auto gas_limit = decode_uint64(*gas_limit_str);
  if (!gas_limit) {
    return json_error("invalid currentGasLimit");
  }
  env.gas_limit = *gas_limit;

  auto number_str = get_string_field(obj, "currentNumber");
  if (!number_str) {
    return json_error("env missing currentNumber");
  }
  auto number = decode_uint64(*number_str);
  if (!number) {
    return json_error("invalid currentNumber");
  }
  env.number = *number;

  auto timestamp_str = get_string_field(obj, "currentTimestamp");
  if (!timestamp_str) {
    return json_error("env missing currentTimestamp");
  }
  auto timestamp = decode_uint64(*timestamp_str);
  if (!timestamp) {
    return json_error("invalid currentTimestamp");
  }
  env.timestamp = *timestamp;

  // Optional fields
  if (auto s = get_string_field(obj, "currentDifficulty")) {
    env.difficulty = decode_uint256(*s);
  }

  auto random_str = get_string_field(obj, "currentRandom");
  if (!random_str) {
    random_str = get_string_field(obj, "prevRandao");
  }
  if (random_str) {
    env.prev_randao = decode_uint256(*random_str);
  }

  if (auto s = get_string_field(obj, "currentBaseFee")) {
    env.base_fee = decode_uint256(*s);
  }

  if (auto s = get_string_field(obj, "currentExcessBlobGas")) {
    env.excess_blob_gas = decode_uint64(*s);
  }

  if (auto s = get_string_field(obj, "parentBeaconBlockRoot")) {
    env.parent_beacon_root = decode_hash(*s);
  }

  // Parent fields for base fee calculation (blockchain tests)
  if (auto s = get_string_field(obj, "parentBaseFee")) {
    env.parent_base_fee = decode_uint256(*s);
  }

  if (auto s = get_string_field(obj, "parentGasUsed")) {
    env.parent_gas_used = decode_uint64(*s);
  }

  if (auto s = get_string_field(obj, "parentGasLimit")) {
    env.parent_gas_limit = decode_uint64(*s);
  }

  // Complex nested fields
  if (auto err = parse_block_hashes(obj, env.block_hashes)) {
    return *err;
  }

  if (auto err = parse_withdrawals(obj, env.withdrawals)) {
    return *err;
  }

  if (auto err = parse_ommers(obj, env.ommers)) {
    return *err;
  }

  return env;
}

// =============================================================================
// TXS PARSING
// =============================================================================

/// Parse transactions from JSON array
std::variant<std::vector<ethereum::Transaction>, ParseError> parse_txs_from_array(
    simdjson::ondemand::array& arr) {
  std::vector<ethereum::Transaction> result;

  for (auto tx_val : arr) {
    auto raw = tx_val.raw_json();
    if (raw.error() != simdjson::SUCCESS) {
      return json_error("failed to get raw JSON for transaction");
    }

    auto tx_result = ethereum::json_decode_transaction(raw.value());
    if (!tx_result.ok()) {
      return json_error("failed to decode transaction: " + tx_result.error_detail);
    }

    result.push_back(std::move(tx_result.value));
  }

  return result;
}

// =============================================================================
// COMBINED INPUT PARSING
// =============================================================================

/// Parse combined stdin input with alloc, env, txs as top-level keys
std::variant<InputData, ParseError> parse_combined_stdin() {
  std::ostringstream ss;
  ss << std::cin.rdbuf();
  const std::string content = ss.str();

  InputData data;

  try {
    simdjson::ondemand::parser parser;
    auto json = simdjson::padded_string(content);
    auto doc = parser.iterate(json);

    auto root = doc.get_object();
    if (root.error() != simdjson::SUCCESS) {
      return json_error("stdin must be a JSON object with alloc, env, txs keys");
    }

    // Parse alloc
    auto alloc_field = root.value().find_field_unordered("alloc");
    if (alloc_field.error() != simdjson::SUCCESS) {
      return json_error("missing 'alloc' key in stdin JSON");
    }
    auto alloc_obj = alloc_field.get_object();
    if (alloc_obj.error() != simdjson::SUCCESS) {
      return json_error("alloc must be an object");
    }
    auto alloc_result = parse_alloc_from_json(alloc_obj.value());
    if (auto* err = std::get_if<ParseError>(&alloc_result)) {
      return *err;
    }
    data.alloc = std::move(std::get<std::map<types::Address, AllocAccount>>(alloc_result));

    // Parse txs
    auto txs_field = root.value().find_field_unordered("txs");
    if (txs_field.error() != simdjson::SUCCESS) {
      return json_error("missing 'txs' key in stdin JSON");
    }
    auto txs_arr = txs_field.get_array();
    if (txs_arr.error() != simdjson::SUCCESS) {
      return json_error("txs must be an array");
    }
    auto txs_result = parse_txs_from_array(txs_arr.value());
    if (auto* err = std::get_if<ParseError>(&txs_result)) {
      return *err;
    }
    data.txs = std::move(std::get<std::vector<ethereum::Transaction>>(txs_result));

    // Parse env
    auto env_field = root.value().find_field_unordered("env");
    if (env_field.error() != simdjson::SUCCESS) {
      return json_error("missing 'env' key in stdin JSON");
    }
    auto env_obj = env_field.get_object();
    if (env_obj.error() != simdjson::SUCCESS) {
      return json_error("env must be an object");
    }
    auto env_result = parse_env_from_json(env_obj.value());
    if (auto* err = std::get_if<ParseError>(&env_result)) {
      return *err;
    }
    data.env = std::move(std::get<EnvInput>(env_result));

  } catch (const simdjson::simdjson_error& e) {
    return json_error(std::string("JSON parse error in stdin: ") + e.what());
  }

  return data;
}

}  // namespace

// =============================================================================
// PUBLIC API
// =============================================================================

std::variant<std::map<types::Address, AllocAccount>, ParseError> parse_alloc(
    const std::string& path) {
  auto content_result = read_input(path);
  if (auto* err = std::get_if<ParseError>(&content_result)) {
    return *err;
  }
  const auto& content = std::get<std::string>(content_result);

  try {
    simdjson::ondemand::parser parser;
    auto json = simdjson::padded_string(content);
    auto doc = parser.iterate(json);

    auto root = doc.get_object();
    if (root.error() != simdjson::SUCCESS) {
      return json_error("alloc must be a JSON object");
    }

    return parse_alloc_from_json(root.value());
  } catch (const simdjson::simdjson_error& e) {
    return json_error(std::string("JSON parse error in alloc: ") + e.what());
  }
}

std::variant<EnvInput, ParseError> parse_env(const std::string& path) {
  auto content_result = read_input(path);
  if (auto* err = std::get_if<ParseError>(&content_result)) {
    return *err;
  }
  const auto& content = std::get<std::string>(content_result);

  try {
    simdjson::ondemand::parser parser;
    auto json = simdjson::padded_string(content);
    auto doc = parser.iterate(json);

    auto root = doc.get_object();
    if (root.error() != simdjson::SUCCESS) {
      return json_error("env must be a JSON object");
    }

    return parse_env_from_json(root.value());
  } catch (const simdjson::simdjson_error& e) {
    return json_error(std::string("JSON parse error in env: ") + e.what());
  }
}

std::variant<std::vector<ethereum::Transaction>, ParseError> parse_txs(const std::string& path) {
  auto content_result = read_input(path);
  if (auto* err = std::get_if<ParseError>(&content_result)) {
    return *err;
  }
  const auto& content = std::get<std::string>(content_result);

  // Empty content = no transactions
  if (content.empty() || content == "[]" || content == "null") {
    return std::vector<ethereum::Transaction>{};
  }

  try {
    simdjson::ondemand::parser parser;
    auto json = simdjson::padded_string(content);
    auto doc = parser.iterate(json);

    auto arr = doc.get_array();
    if (arr.error() != simdjson::SUCCESS) {
      return json_error("txs must be a JSON array");
    }

    return parse_txs_from_array(arr.value());
  } catch (const simdjson::simdjson_error& e) {
    return json_error(std::string("JSON parse error in txs: ") + e.what());
  }
}

std::variant<InputData, ParseError> parse_inputs(const std::string& alloc_path,
                                                 const std::string& env_path,
                                                 const std::string& txs_path) {
  // When all inputs are stdin, parse combined JSON format
  if (alloc_path == "stdin" && env_path == "stdin" && txs_path == "stdin") {
    return parse_combined_stdin();
  }

  InputData data;

  auto alloc_result = parse_alloc(alloc_path);
  if (auto* err = std::get_if<ParseError>(&alloc_result)) {
    return *err;
  }
  data.alloc = std::move(std::get<std::map<types::Address, AllocAccount>>(alloc_result));

  auto env_result = parse_env(env_path);
  if (auto* err = std::get_if<ParseError>(&env_result)) {
    return *err;
  }
  data.env = std::move(std::get<EnvInput>(env_result));

  auto txs_result = parse_txs(txs_path);
  if (auto* err = std::get_if<ParseError>(&txs_result)) {
    return *err;
  }
  data.txs = std::move(std::get<std::vector<ethereum::Transaction>>(txs_result));

  return data;
}

}  // namespace div0::cli
