#include "div0/crypto/keccak256.h"
#include "div0/crypto/secp256k1.h"
#include "div0/ethereum/transaction/rlp.h"
#include "div0/rlp/decode.h"
#include "div0/rlp/encode.h"

#ifndef DIV0_BARE_METAL
#include "div0/ethereum/transaction/json.h"
#endif

namespace div0::ethereum {

using rlp::RlpDecoder;
using rlp::RlpEncoder;

// =============================================================================
// Eip7702Tx RLP Encoding
// =============================================================================

types::Bytes rlp_encode(const Eip7702Tx& tx) {
  // Envelope: 0x04 || RLP([chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
  //                        gas_limit, to, value, data, access_list, authorization_list,
  //                        y_parity, r, s])

  // Phase 1: Calculate lengths
  const size_t chain_id_len = RlpEncoder::encoded_length(tx.chain_id);
  const size_t nonce_len = RlpEncoder::encoded_length(tx.nonce);
  const size_t max_priority_fee_len = RlpEncoder::encoded_length(tx.max_priority_fee_per_gas);
  const size_t max_fee_len = RlpEncoder::encoded_length(tx.max_fee_per_gas);
  const size_t gas_limit_len = RlpEncoder::encoded_length(tx.gas_limit);
  const size_t to_len = RlpEncoder::encoded_length_address();  // Required for 7702 txs
  const size_t value_len = RlpEncoder::encoded_length(tx.value);
  const size_t data_len = RlpEncoder::encoded_length(std::span(tx.data));
  const size_t access_list_len = rlp_encoded_length(tx.access_list);
  const size_t authorization_list_len = rlp_encoded_length(tx.authorization_list);
  const size_t y_parity_len = RlpEncoder::encoded_length(tx.y_parity);
  const size_t r_len = RlpEncoder::encoded_length(tx.r);
  const size_t s_len = RlpEncoder::encoded_length(tx.s);

  const size_t payload = chain_id_len + nonce_len + max_priority_fee_len + max_fee_len +
                         gas_limit_len + to_len + value_len + data_len + access_list_len +
                         authorization_list_len + y_parity_len + r_len + s_len;
  const size_t rlp_len = RlpEncoder::list_length(payload);
  const size_t total = 1 + rlp_len;  // 0x04 prefix + RLP

  // Phase 2: Encode
  types::Bytes result(total);
  result[0] = 0x04;  // Type prefix

  RlpEncoder enc(std::span(result).subspan(1));
  enc.start_list(payload);
  enc.encode(tx.chain_id);
  enc.encode(tx.nonce);
  enc.encode(tx.max_priority_fee_per_gas);
  enc.encode(tx.max_fee_per_gas);
  enc.encode(tx.gas_limit);
  enc.encode_address(tx.to);
  enc.encode(tx.value);
  enc.encode(std::span(tx.data));
  rlp_encode_access_list(enc, tx.access_list);
  rlp_encode_authorization_list(enc, tx.authorization_list);
  enc.encode(tx.y_parity);
  enc.encode(tx.r);
  enc.encode(tx.s);

  return result;
}

// =============================================================================
// Eip7702Tx RLP Decoding
// =============================================================================

TxDecodeResult<Eip7702Tx> rlp_decode_eip7702_tx(std::span<const uint8_t> data) {
  // Note: data should be the RLP payload AFTER the 0x04 type byte

  if (data.empty()) {
    return {.error = TxDecodeError::EmptyInput};
  }

  RlpDecoder dec(data);

  // Decode list header
  auto list_header = dec.decode_list_header();
  if (!list_header.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }

  Eip7702Tx tx;

  // chain_id
  auto chain_id_result = dec.decode_uint64();
  if (!chain_id_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.chain_id = chain_id_result.value;

  // nonce
  auto nonce_result = dec.decode_uint64();
  if (!nonce_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.nonce = nonce_result.value;

  // max_priority_fee_per_gas
  auto max_priority_fee_result = dec.decode_uint256();
  if (!max_priority_fee_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.max_priority_fee_per_gas = max_priority_fee_result.value;

  // max_fee_per_gas
  auto max_fee_result = dec.decode_uint256();
  if (!max_fee_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.max_fee_per_gas = max_fee_result.value;

  // gas_limit
  auto gas_limit_result = dec.decode_uint64();
  if (!gas_limit_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.gas_limit = gas_limit_result.value;

  // to (required for 7702 transactions)
  auto to_result = dec.decode_address();
  if (!to_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.to = to_result.value;

  // value
  auto value_result = dec.decode_uint256();
  if (!value_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.value = value_result.value;

  // data
  auto data_result = dec.decode_bytes();
  if (!data_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.data = types::Bytes(data_result.value.begin(), data_result.value.end());

  // access_list
  auto access_list_error = rlp_decode_access_list(dec, tx.access_list);
  if (access_list_error != TxDecodeError::Success) {
    return {.error = access_list_error};
  }

  // authorization_list
  auto auth_list_error = rlp_decode_authorization_list(dec, tx.authorization_list);
  if (auth_list_error != TxDecodeError::Success) {
    return {.error = auth_list_error};
  }

  // y_parity
  auto y_parity_result = dec.decode_uint64();
  if (!y_parity_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.y_parity = static_cast<uint8_t>(y_parity_result.value);

  // r
  auto r_result = dec.decode_uint256();
  if (!r_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.r = r_result.value;

  // s
  auto s_result = dec.decode_uint256();
  if (!s_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.s = s_result.value;

  return {.value = std::move(tx)};
}

// =============================================================================
// Eip7702Tx Transaction Hash
// =============================================================================

types::Hash tx_hash(const Eip7702Tx& tx) {
  if (tx.cached_hash) {
    return *tx.cached_hash;
  }

  auto encoded = rlp_encode(tx);
  auto hash = crypto::keccak256(std::span<const uint8_t>(encoded));
  tx.cached_hash = hash;
  return hash;
}

// =============================================================================
// Eip7702Tx Signing Hash (for sender recovery)
// =============================================================================

types::Hash signing_hash(const Eip7702Tx& tx) {
  // keccak256(0x04 || rlp([chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
  //                        gas_limit, to, value, data, access_list, authorization_list]))

  // Calculate payload length (without signature fields)
  const size_t chain_id_len = RlpEncoder::encoded_length(tx.chain_id);
  const size_t nonce_len = RlpEncoder::encoded_length(tx.nonce);
  const size_t max_priority_fee_len = RlpEncoder::encoded_length(tx.max_priority_fee_per_gas);
  const size_t max_fee_len = RlpEncoder::encoded_length(tx.max_fee_per_gas);
  const size_t gas_limit_len = RlpEncoder::encoded_length(tx.gas_limit);
  const size_t to_len = RlpEncoder::encoded_length_address();
  const size_t value_len = RlpEncoder::encoded_length(tx.value);
  const size_t data_len = RlpEncoder::encoded_length(std::span<const uint8_t>(tx.data));
  const size_t access_list_len = rlp_encoded_length(tx.access_list);
  const size_t authorization_list_len = rlp_encoded_length(tx.authorization_list);

  const size_t payload = chain_id_len + nonce_len + max_priority_fee_len + max_fee_len +
                         gas_limit_len + to_len + value_len + data_len + access_list_len +
                         authorization_list_len;
  const size_t rlp_len = RlpEncoder::list_length(payload);
  const size_t total = 1 + rlp_len;  // 0x04 prefix + RLP

  // Encode
  types::Bytes result(total);
  result[0] = 0x04;

  RlpEncoder enc(std::span(result).subspan(1));
  enc.start_list(payload);
  enc.encode(tx.chain_id);
  enc.encode(tx.nonce);
  enc.encode(tx.max_priority_fee_per_gas);
  enc.encode(tx.max_fee_per_gas);
  enc.encode(tx.gas_limit);
  enc.encode_address(tx.to);
  enc.encode(tx.value);
  enc.encode(std::span(tx.data));
  rlp_encode_access_list(enc, tx.access_list);
  rlp_encode_authorization_list(enc, tx.authorization_list);

  return crypto::keccak256(std::span<const uint8_t>(result));
}

// =============================================================================
// Eip7702Tx Sender Recovery
// =============================================================================

std::optional<types::Address> recover_sender(const Eip7702Tx& tx, crypto::Secp256k1Context& ctx) {
  if (tx.cached_sender) {
    return tx.cached_sender;
  }

  auto hash = signing_hash(tx);

  // For typed transactions, ecrecover uses y_parity directly as recovery_id
  // We need to convert to the legacy v format: v = 27 + y_parity
  const uint64_t v = 27 + tx.y_parity;
  auto sender = ctx.ecrecover(hash.to_uint256(), v, tx.r, tx.s, std::nullopt);

  if (sender) {
    tx.cached_sender = *sender;
  }

  return sender;
}

#ifndef DIV0_BARE_METAL

// =============================================================================
// Eip7702Tx JSON Encoding
// =============================================================================

std::string json_encode(const Eip7702Tx& tx) {
  std::ostringstream ss;
  ss << '{';

  json_append_key(ss, "type");
  json_append_string(ss, "0x4");
  ss << ',';

  json_append_key(ss, "chainId");
  json_append_string(ss, hex::encode_uint64(tx.chain_id));
  ss << ',';

  json_append_key(ss, "nonce");
  json_append_string(ss, hex::encode_uint64(tx.nonce));
  ss << ',';

  json_append_key(ss, "maxPriorityFeePerGas");
  json_append_string(ss, hex::encode_uint256(tx.max_priority_fee_per_gas));
  ss << ',';

  json_append_key(ss, "maxFeePerGas");
  json_append_string(ss, hex::encode_uint256(tx.max_fee_per_gas));
  ss << ',';

  json_append_key(ss, "gas");
  json_append_string(ss, hex::encode_uint64(tx.gas_limit));
  ss << ',';

  json_append_key(ss, "to");
  json_append_string(ss, hex::encode_address(tx.to));
  ss << ',';

  json_append_key(ss, "value");
  json_append_string(ss, hex::encode_uint256(tx.value));
  ss << ',';

  json_append_key(ss, "input");
  json_append_string(ss, hex::encode_bytes(tx.data));
  ss << ',';

  json_append_key(ss, "accessList");
  json_encode_access_list(ss, tx.access_list);
  ss << ',';

  json_append_key(ss, "authorizationList");
  json_encode_authorization_list(ss, tx.authorization_list);
  ss << ',';

  json_append_key(ss, "yParity");
  json_append_string(ss, hex::encode_uint64(tx.y_parity));
  ss << ',';

  json_append_key(ss, "r");
  json_append_string(ss, hex::encode_uint256(tx.r));
  ss << ',';

  json_append_key(ss, "s");
  json_append_string(ss, hex::encode_uint256(tx.s));

  ss << '}';
  return ss.str();
}

// =============================================================================
// Eip7702Tx JSON Decoding
// =============================================================================

JsonDecodeResult<Eip7702Tx> json_decode_eip7702_tx(std::string_view json) {
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

  Eip7702Tx tx;

  // chainId (required)
  auto chain_id = json_parse_hex_uint64_field(obj.value(), "chainId");
  if (!chain_id) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "chainId"};
  }
  tx.chain_id = *chain_id;

  // nonce (required)
  auto nonce = json_parse_hex_uint64_field(obj.value(), "nonce");
  if (!nonce) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "nonce"};
  }
  tx.nonce = *nonce;

  // maxPriorityFeePerGas (required)
  auto max_priority_fee = json_parse_hex_uint256_field(obj.value(), "maxPriorityFeePerGas");
  if (!max_priority_fee) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "maxPriorityFeePerGas"};
  }
  tx.max_priority_fee_per_gas = *max_priority_fee;

  // maxFeePerGas (required)
  auto max_fee = json_parse_hex_uint256_field(obj.value(), "maxFeePerGas");
  if (!max_fee) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "maxFeePerGas"};
  }
  tx.max_fee_per_gas = *max_fee;

  // gas (required)
  auto gas = json_parse_hex_uint64_field(obj.value(), "gas");
  if (!gas) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "gas"};
  }
  tx.gas_limit = *gas;

  // to (required for SetCode tx)
  auto to = json_parse_hex_address_field(obj.value(), "to");
  if (!to) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "to"};
  }
  tx.to = *to;

  // value (required)
  auto value = json_parse_hex_uint256_field(obj.value(), "value");
  if (!value) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "value"};
  }
  tx.value = *value;

  // input/data (required)
  auto input = json_parse_input_field(obj.value());
  if (!input) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "input"};
  }
  tx.data = std::move(*input);

  // accessList (required, may be empty)
  auto access_list = json_parse_access_list_field(obj.value());
  if (!access_list) {
    return {.error = JsonDecodeError::InvalidField, .error_detail = "accessList"};
  }
  tx.access_list = std::move(*access_list);

  // authorizationList (required)
  auto auth_list_field = obj.value().find_field_unordered("authorizationList");
  if (auth_list_field.error() != simdjson::SUCCESS) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "authorizationList"};
  }
  auto auth_list_arr = auth_list_field.get_array();
  if (auth_list_arr.error() != simdjson::SUCCESS) {
    return {.error = JsonDecodeError::InvalidField, .error_detail = "authorizationList"};
  }
  auto parsed_auth_list = json_parse_authorization_list(auth_list_arr.value());
  if (!parsed_auth_list) {
    return {.error = JsonDecodeError::InvalidField, .error_detail = "authorizationList"};
  }
  tx.authorization_list = std::move(*parsed_auth_list);

  // yParity (required)
  auto y_parity = json_parse_hex_uint64_field(obj.value(), "yParity");
  if (!y_parity) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "yParity"};
  }
  tx.y_parity = static_cast<uint8_t>(*y_parity);

  // r (required)
  auto r = json_parse_hex_uint256_field(obj.value(), "r");
  if (!r) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "r"};
  }
  tx.r = *r;

  // s (required)
  auto s = json_parse_hex_uint256_field(obj.value(), "s");
  if (!s) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "s"};
  }
  tx.s = *s;

  return {.value = std::move(tx), .error = JsonDecodeError::Success, .error_detail = {}};
}

#endif  // DIV0_BARE_METAL

}  // namespace div0::ethereum
