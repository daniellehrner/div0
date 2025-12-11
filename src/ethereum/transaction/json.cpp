// JSON transaction encoding/decoding is not available in bare-metal builds
#ifndef DIV0_BARE_METAL

#include "div0/ethereum/transaction/json.h"

#include <sstream>

namespace div0::ethereum {

using types::Address;
using types::Bytes;
using types::Uint256;

// =============================================================================
// JSON String Building Helpers
// =============================================================================

void json_append_key(std::ostringstream& ss, std::string_view key) { ss << '"' << key << "\":"; }

void json_append_string(std::ostringstream& ss, std::string_view value) {
  ss << '"' << value << '"';
}

void json_append_null(std::ostringstream& ss) { ss << "null"; }

// =============================================================================
// Access List JSON Encoding
// =============================================================================

void json_encode_access_list(std::ostringstream& ss, const AccessList& list) {
  ss << '[';
  bool first_entry = true;
  for (const auto& entry : list) {
    if (!first_entry) {
      ss << ',';
    }
    first_entry = false;

    ss << "{\"address\":";
    json_append_string(ss, hex::encode_address(entry.address));
    ss << ",\"storageKeys\":[";

    bool first_key = true;
    for (const auto& key : entry.storage_keys) {
      if (!first_key) {
        ss << ',';
      }
      first_key = false;
      json_append_string(ss, encode_storage_slot(key));
    }
    ss << "]}";
  }
  ss << ']';
}

// =============================================================================
// Authorization List JSON Encoding (EIP-7702)
// =============================================================================

void json_encode_authorization_list(std::ostringstream& ss, const AuthorizationList& list) {
  ss << '[';
  bool first = true;
  for (const auto& auth : list) {
    if (!first) {
      ss << ',';
    }
    first = false;

    ss << '{';
    json_append_key(ss, "chainId");
    json_append_string(ss, hex::encode_uint64(auth.chain_id));
    ss << ',';
    json_append_key(ss, "address");
    json_append_string(ss, hex::encode_address(auth.address));
    ss << ',';
    json_append_key(ss, "nonce");
    json_append_string(ss, hex::encode_uint64(auth.nonce));
    ss << ',';
    json_append_key(ss, "yParity");
    json_append_string(ss, hex::encode_uint64(auth.y_parity));
    ss << ',';
    json_append_key(ss, "r");
    json_append_string(ss, hex::encode_uint256(auth.r));
    ss << ',';
    json_append_key(ss, "s");
    json_append_string(ss, hex::encode_uint256(auth.s));
    ss << '}';
  }
  ss << ']';
}

// =============================================================================
// JSON Decoding Helpers
// =============================================================================

std::optional<std::string_view> json_get_string_field(simdjson::ondemand::object& obj,
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

std::optional<uint64_t> json_parse_hex_uint64_field(simdjson::ondemand::object& obj,
                                                    std::string_view key) {
  auto str_opt = json_get_string_field(obj, key);
  if (!str_opt) {
    return std::nullopt;
  }
  return hex::decode_uint64(*str_opt);
}

std::optional<Uint256> json_parse_hex_uint256_field(simdjson::ondemand::object& obj,
                                                    std::string_view key) {
  auto str_opt = json_get_string_field(obj, key);
  if (!str_opt) {
    return std::nullopt;
  }
  return hex::decode_uint256(*str_opt);
}

std::optional<Bytes> json_parse_hex_bytes_field(simdjson::ondemand::object& obj,
                                                std::string_view key) {
  auto str_opt = json_get_string_field(obj, key);
  if (!str_opt) {
    return std::nullopt;
  }
  return hex::decode_bytes(*str_opt);
}

std::optional<Address> json_parse_hex_address_field(simdjson::ondemand::object& obj,
                                                    std::string_view key) {
  auto str_opt = json_get_string_field(obj, key);
  if (!str_opt) {
    return std::nullopt;
  }
  return hex::decode_address(*str_opt);
}

std::optional<std::optional<Address>> json_parse_optional_address_field(
    simdjson::ondemand::object& obj, std::string_view key) {
  auto field = obj.find_field_unordered(key);
  if (field.error() != simdjson::SUCCESS) {
    return std::optional<Address>{std::nullopt};  // Field not present
  }

  // Check for null
  if (field.is_null()) {
    return std::optional<Address>{std::nullopt};  // Null value
  }

  auto str = field.get_string();
  if (str.error() != simdjson::SUCCESS) {
    return std::nullopt;  // Error - field present but not a string
  }

  auto addr = hex::decode_address(str.value());
  if (!addr) {
    return std::nullopt;  // Invalid hex
  }

  return addr;
}

std::optional<AccessList> json_parse_access_list(simdjson::ondemand::array& arr) {
  AccessList result;

  for (auto entry_value : arr) {
    auto entry_obj = entry_value.get_object();
    if (entry_obj.error() != simdjson::SUCCESS) {
      return std::nullopt;
    }

    AccessListEntry entry;

    // Parse address
    auto addr_str = json_get_string_field(entry_obj.value(), "address");
    if (!addr_str) {
      return std::nullopt;
    }
    auto addr = hex::decode_address(*addr_str);
    if (!addr) {
      return std::nullopt;
    }
    entry.address = *addr;

    // Parse storage keys
    auto keys_field = entry_obj.value().find_field_unordered("storageKeys");
    if (keys_field.error() == simdjson::SUCCESS) {
      auto keys_arr = keys_field.get_array();
      if (keys_arr.error() == simdjson::SUCCESS) {
        for (auto key_value : keys_arr.value()) {
          auto key_str = key_value.get_string();
          if (key_str.error() != simdjson::SUCCESS) {
            return std::nullopt;
          }
          auto key = decode_storage_slot(key_str.value());
          if (!key) {
            return std::nullopt;
          }
          entry.storage_keys.push_back(*key);
        }
      }
    }

    result.push_back(std::move(entry));
  }

  return result;
}

std::optional<AuthorizationList> json_parse_authorization_list(simdjson::ondemand::array& arr) {
  AuthorizationList result;

  for (auto auth_value : arr) {
    auto auth_obj = auth_value.get_object();
    if (auth_obj.error() != simdjson::SUCCESS) {
      return std::nullopt;
    }

    Authorization auth;

    // chainId
    auto chain_id_str = json_get_string_field(auth_obj.value(), "chainId");
    if (!chain_id_str) {
      return std::nullopt;
    }
    auto chain_id = hex::decode_uint64(*chain_id_str);
    if (!chain_id) {
      return std::nullopt;
    }
    auth.chain_id = *chain_id;

    // address
    auto addr_str = json_get_string_field(auth_obj.value(), "address");
    if (!addr_str) {
      return std::nullopt;
    }
    auto addr = hex::decode_address(*addr_str);
    if (!addr) {
      return std::nullopt;
    }
    auth.address = *addr;

    // nonce
    auto nonce_str = json_get_string_field(auth_obj.value(), "nonce");
    if (!nonce_str) {
      return std::nullopt;
    }
    auto nonce = hex::decode_uint64(*nonce_str);
    if (!nonce) {
      return std::nullopt;
    }
    auth.nonce = *nonce;

    // yParity
    auto y_parity_str = json_get_string_field(auth_obj.value(), "yParity");
    if (!y_parity_str) {
      return std::nullopt;
    }
    auto y_parity = hex::decode_uint64(*y_parity_str);
    if (!y_parity) {
      return std::nullopt;
    }
    auth.y_parity = static_cast<uint8_t>(*y_parity);

    // r
    auto r_str = json_get_string_field(auth_obj.value(), "r");
    if (!r_str) {
      return std::nullopt;
    }
    auto r = hex::decode_uint256(*r_str);
    if (!r) {
      return std::nullopt;
    }
    auth.r = *r;

    // s
    auto s_str = json_get_string_field(auth_obj.value(), "s");
    if (!s_str) {
      return std::nullopt;
    }
    auto s = hex::decode_uint256(*s_str);
    if (!s) {
      return std::nullopt;
    }
    auth.s = *s;

    result.push_back(auth);
  }

  return result;
}

// =============================================================================
// Common Decoding Patterns
// =============================================================================

std::optional<AccessList> json_parse_access_list_field(simdjson::ondemand::object& obj) {
  auto access_list_field = obj.find_field_unordered("accessList");
  if (access_list_field.error() != simdjson::SUCCESS) {
    return AccessList{};  // Missing field is OK, return empty list
  }
  auto access_list_arr = access_list_field.get_array();
  if (access_list_arr.error() != simdjson::SUCCESS) {
    return std::nullopt;  // Present but not an array
  }
  return json_parse_access_list(access_list_arr.value());
}

std::optional<Bytes> json_parse_input_field(simdjson::ondemand::object& obj) {
  auto input = json_parse_hex_bytes_field(obj, "input");
  if (input) {
    return input;
  }
  return json_parse_hex_bytes_field(obj, "data");
}

// =============================================================================
// JSON Encoding - Transaction (variant wrapper)
// =============================================================================

std::string json_encode(const Transaction& tx) {
  return std::visit([](const auto& inner) { return json_encode(inner); }, tx.variant());
}

// =============================================================================
// JSON Decoding - Transaction (with type detection)
// =============================================================================

JsonDecodeResult<Transaction> json_decode_transaction(std::string_view json) {
  simdjson::ondemand::parser parser;
  const simdjson::padded_string padded(json);

  auto doc = parser.iterate(padded);
  if (doc.error() != simdjson::SUCCESS) {
    return {.error = JsonDecodeError::ParseError, .error_detail = "Invalid JSON"};
  }

  auto obj = doc.get_object();
  if (obj.error() != simdjson::SUCCESS) {
    return {.error = JsonDecodeError::ParseError, .error_detail = "Expected JSON object"};
  }

  // Detect transaction type
  TxType tx_type = TxType::Legacy;  // Default to legacy

  auto type_field = obj.value().find_field_unordered("type");
  if (type_field.error() == simdjson::SUCCESS) {
    auto type_str = type_field.get_string();
    if (type_str.error() == simdjson::SUCCESS) {
      auto type_val = hex::decode_uint64(type_str.value());
      if (type_val && *type_val <= 4) {
        tx_type = static_cast<TxType>(*type_val);
      } else if (type_val && *type_val > 4) {
        return {.error = JsonDecodeError::InvalidType, .error_detail = "Unknown transaction type"};
      }
    }
  }

  // Decode based on detected type
  switch (tx_type) {
    case TxType::Legacy: {
      auto result = json_decode_legacy_tx(json);
      if (!result.ok()) {
        return {.error = result.error, .error_detail = result.error_detail};
      }
      return {.value = Transaction(std::move(result.value)),
              .error = JsonDecodeError::Success,
              .error_detail = {}};
    }
    case TxType::AccessList: {
      auto result = json_decode_eip2930_tx(json);
      if (!result.ok()) {
        return {.error = result.error, .error_detail = result.error_detail};
      }
      return {.value = Transaction(std::move(result.value)),
              .error = JsonDecodeError::Success,
              .error_detail = {}};
    }
    case TxType::DynamicFee: {
      auto result = json_decode_eip1559_tx(json);
      if (!result.ok()) {
        return {.error = result.error, .error_detail = result.error_detail};
      }
      return {.value = Transaction(std::move(result.value)),
              .error = JsonDecodeError::Success,
              .error_detail = {}};
    }
    case TxType::Blob: {
      auto result = json_decode_eip4844_tx(json);
      if (!result.ok()) {
        return {.error = result.error, .error_detail = result.error_detail};
      }
      return {.value = Transaction(std::move(result.value)),
              .error = JsonDecodeError::Success,
              .error_detail = {}};
    }
    case TxType::SetCode: {
      auto result = json_decode_eip7702_tx(json);
      if (!result.ok()) {
        return {.error = result.error, .error_detail = result.error_detail};
      }
      return {.value = Transaction(std::move(result.value)),
              .error = JsonDecodeError::Success,
              .error_detail = {}};
    }
  }

  return {.error = JsonDecodeError::InvalidType, .error_detail = "Unexpected transaction type"};
}

}  // namespace div0::ethereum

#endif  // DIV0_BARE_METAL
