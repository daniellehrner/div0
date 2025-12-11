#ifndef DIV0_ETHEREUM_TRANSACTION_JSON_H
#define DIV0_ETHEREUM_TRANSACTION_JSON_H

// JSON transaction encoding/decoding is not available in bare-metal builds
#ifndef DIV0_BARE_METAL

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "div0/ethereum/storage_slot.h"
#include "div0/ethereum/transaction/eip1559_tx.h"
#include "div0/ethereum/transaction/eip2930_tx.h"
#include "div0/ethereum/transaction/eip4844_tx.h"
#include "div0/ethereum/transaction/eip7702_tx.h"
#include "div0/ethereum/transaction/legacy_tx.h"
#include "div0/ethereum/transaction/transaction.h"
#include "div0/utils/hex.h"

// simdjson has various warnings in its template code
// NOLINTBEGIN(clang-diagnostic-sign-conversion,clang-diagnostic-format-nonliteral)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#include <simdjson.h>
#pragma clang diagnostic pop
// NOLINTEND(clang-diagnostic-sign-conversion,clang-diagnostic-format-nonliteral)

namespace div0::ethereum {

// =============================================================================
// Error Types
// =============================================================================

/**
 * @brief JSON decode error codes.
 */
enum class JsonDecodeError : uint8_t {
  Success,
  ParseError,    // Invalid JSON syntax
  MissingField,  // Required field not present
  InvalidField,  // Field has wrong type or format
  InvalidType,   // Unknown or invalid transaction type
  InvalidHex,    // Malformed hex string
  Overflow,      // Value too large for target type
  TypeMismatch,  // Field present but wrong transaction type
};

/**
 * @brief Result of a JSON decode operation.
 */
template <typename T>
struct JsonDecodeResult {
  T value{};
  JsonDecodeError error{JsonDecodeError::Success};
  std::string error_detail;  // Optional detail about which field failed

  [[nodiscard]] bool ok() const noexcept { return error == JsonDecodeError::Success; }
  explicit operator bool() const noexcept { return ok(); }
};

// =============================================================================
// JSON Encoding - Transaction to JSON string
// =============================================================================

/**
 * @brief Encode a legacy transaction to JSON.
 *
 * Output format follows go-ethereum conventions:
 * - All numeric values are "0x" prefixed hex (quantity encoding)
 * - Addresses are "0x" prefixed 40-char lowercase hex
 * - Byte arrays are "0x" prefixed even-length hex
 * - null for missing optional fields (to for contract creation)
 */
[[nodiscard]] std::string json_encode(const LegacyTx& tx);
[[nodiscard]] std::string json_encode(const Eip2930Tx& tx);
[[nodiscard]] std::string json_encode(const Eip1559Tx& tx);
[[nodiscard]] std::string json_encode(const Eip4844Tx& tx);
[[nodiscard]] std::string json_encode(const Eip7702Tx& tx);
[[nodiscard]] std::string json_encode(const Transaction& tx);

// =============================================================================
// JSON Decoding - JSON string to Transaction
// =============================================================================

/**
 * @brief Decode a legacy transaction from JSON.
 * @param json JSON string (must be valid JSON object)
 */
[[nodiscard]] JsonDecodeResult<LegacyTx> json_decode_legacy_tx(std::string_view json);
[[nodiscard]] JsonDecodeResult<Eip2930Tx> json_decode_eip2930_tx(std::string_view json);
[[nodiscard]] JsonDecodeResult<Eip1559Tx> json_decode_eip1559_tx(std::string_view json);
[[nodiscard]] JsonDecodeResult<Eip4844Tx> json_decode_eip4844_tx(std::string_view json);
[[nodiscard]] JsonDecodeResult<Eip7702Tx> json_decode_eip7702_tx(std::string_view json);

/**
 * @brief Decode any transaction type with automatic type detection.
 *
 * Reads the "type" field to determine transaction type:
 * - "0x0" or missing: Legacy
 * - "0x1": EIP-2930 Access List
 * - "0x2": EIP-1559 Dynamic Fee
 * - "0x3": EIP-4844 Blob
 * - "0x4": EIP-7702 SetCode
 */
[[nodiscard]] JsonDecodeResult<Transaction> json_decode_transaction(std::string_view json);

// =============================================================================
// JSON Helper Function Declarations (for use in tx files)
// =============================================================================

// JSON string building helpers
void json_append_key(std::ostringstream& ss, std::string_view key);
void json_append_string(std::ostringstream& ss, std::string_view value);
void json_append_null(std::ostringstream& ss);

// JSON encoding helpers for complex types
void json_encode_access_list(std::ostringstream& ss, const AccessList& list);
void json_encode_authorization_list(std::ostringstream& ss, const AuthorizationList& list);

// JSON decoding helpers
[[nodiscard]] std::optional<std::string_view> json_get_string_field(simdjson::ondemand::object& obj,
                                                                    std::string_view key);
[[nodiscard]] std::optional<uint64_t> json_parse_hex_uint64_field(simdjson::ondemand::object& obj,
                                                                  std::string_view key);
[[nodiscard]] std::optional<types::Uint256> json_parse_hex_uint256_field(
    simdjson::ondemand::object& obj, std::string_view key);
[[nodiscard]] std::optional<types::Bytes> json_parse_hex_bytes_field(
    simdjson::ondemand::object& obj, std::string_view key);
[[nodiscard]] std::optional<types::Address> json_parse_hex_address_field(
    simdjson::ondemand::object& obj, std::string_view key);
[[nodiscard]] std::optional<std::optional<types::Address>> json_parse_optional_address_field(
    simdjson::ondemand::object& obj, std::string_view key);
[[nodiscard]] std::optional<AccessList> json_parse_access_list(simdjson::ondemand::array& arr);
[[nodiscard]] std::optional<AuthorizationList> json_parse_authorization_list(
    simdjson::ondemand::array& arr);
[[nodiscard]] std::optional<AccessList> json_parse_access_list_field(
    simdjson::ondemand::object& obj);
[[nodiscard]] std::optional<types::Bytes> json_parse_input_field(simdjson::ondemand::object& obj);

}  // namespace div0::ethereum

#endif  // DIV0_BARE_METAL

#endif  // DIV0_ETHEREUM_TRANSACTION_JSON_H
