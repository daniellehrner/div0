#include "div0/crypto/keccak256.h"
#include "div0/crypto/secp256k1.h"
#include "div0/ethereum/transaction/json.h"
#include "div0/ethereum/transaction/rlp.h"
#include "div0/rlp/decode.h"
#include "div0/rlp/encode.h"

namespace div0::ethereum {

using rlp::RlpDecoder;
using rlp::RlpEncoder;

// =============================================================================
// Eip2930Tx RLP Encoding
// =============================================================================

types::Bytes rlp_encode(const Eip2930Tx& tx) {
  // Envelope: 0x01 || RLP([chain_id, nonce, gas_price, gas_limit, to, value, data,
  //                        access_list, y_parity, r, s])

  // Phase 1: Calculate lengths
  const size_t chain_id_len = RlpEncoder::encoded_length(tx.chain_id);
  const size_t nonce_len = RlpEncoder::encoded_length(tx.nonce);
  const size_t gas_price_len = RlpEncoder::encoded_length(tx.gas_price);
  const size_t gas_limit_len = RlpEncoder::encoded_length(tx.gas_limit);
  const size_t to_len = tx.to ? RlpEncoder::encoded_length_address()
                              : RlpEncoder::encoded_length(std::span<const uint8_t>{});
  const size_t value_len = RlpEncoder::encoded_length(tx.value);
  const size_t data_len = RlpEncoder::encoded_length(std::span<const uint8_t>(tx.data));
  const size_t access_list_len = rlp_encoded_length(tx.access_list);
  const size_t y_parity_len = RlpEncoder::encoded_length(static_cast<uint64_t>(tx.y_parity));
  const size_t r_len = RlpEncoder::encoded_length(tx.r);
  const size_t s_len = RlpEncoder::encoded_length(tx.s);

  const size_t payload = chain_id_len + nonce_len + gas_price_len + gas_limit_len + to_len +
                         value_len + data_len + access_list_len + y_parity_len + r_len + s_len;
  const size_t rlp_len = RlpEncoder::list_length(payload);
  const size_t total = 1 + rlp_len;  // 0x01 prefix + RLP

  // Phase 2: Encode
  types::Bytes result(total);
  result[0] = 0x01;  // Type prefix

  RlpEncoder enc(std::span(result).subspan(1));
  enc.start_list(payload);
  enc.encode(tx.chain_id);
  enc.encode(tx.nonce);
  enc.encode(tx.gas_price);
  enc.encode(tx.gas_limit);
  if (tx.to) {
    enc.encode_address(*tx.to);
  } else {
    enc.encode(std::span<const uint8_t>{});
  }
  enc.encode(tx.value);
  enc.encode(std::span<const uint8_t>(tx.data));
  rlp_encode_access_list(enc, tx.access_list);
  enc.encode(tx.y_parity);
  enc.encode(tx.r);
  enc.encode(tx.s);

  return result;
}

// =============================================================================
// Eip2930Tx RLP Decoding
// =============================================================================

TxDecodeResult<Eip2930Tx> rlp_decode_eip2930_tx(std::span<const uint8_t> data) {
  // Note: data should be the RLP payload AFTER the 0x01 type byte

  if (data.empty()) {
    return {.error = TxDecodeError::EmptyInput};
  }

  RlpDecoder dec(data);

  // Decode list header
  auto list_header = dec.decode_list_header();
  if (!list_header.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }

  Eip2930Tx tx;

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

  // gas_price
  auto gas_price_result = dec.decode_uint256();
  if (!gas_price_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.gas_price = gas_price_result.value;

  // gas_limit
  auto gas_limit_result = dec.decode_uint64();
  if (!gas_limit_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.gas_limit = gas_limit_result.value;

  // to (optional)
  auto to_result = rlp_decode_optional_address(dec);
  if (!to_result) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.to = *to_result;

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
// Eip2930Tx Transaction Hash
// =============================================================================

types::Hash tx_hash(const Eip2930Tx& tx) {
  if (tx.cached_hash) {
    return *tx.cached_hash;
  }

  auto encoded = rlp_encode(tx);
  auto hash = crypto::keccak256(std::span<const uint8_t>(encoded));
  tx.cached_hash = hash;
  return hash;
}

// =============================================================================
// Eip2930Tx Signing Hash (for sender recovery)
// =============================================================================

types::Hash signing_hash(const Eip2930Tx& tx) {
  // keccak256(0x01 || rlp([chain_id, nonce, gas_price, gas_limit, to, value, data, access_list]))

  // Calculate payload length (without signature fields)
  const size_t chain_id_len = RlpEncoder::encoded_length(tx.chain_id);
  const size_t nonce_len = RlpEncoder::encoded_length(tx.nonce);
  const size_t gas_price_len = RlpEncoder::encoded_length(tx.gas_price);
  const size_t gas_limit_len = RlpEncoder::encoded_length(tx.gas_limit);
  const size_t to_len = tx.to ? RlpEncoder::encoded_length_address()
                              : RlpEncoder::encoded_length(std::span<const uint8_t>{});
  const size_t value_len = RlpEncoder::encoded_length(tx.value);
  const size_t data_len = RlpEncoder::encoded_length(std::span<const uint8_t>(tx.data));
  const size_t access_list_len = rlp_encoded_length(tx.access_list);

  const size_t payload = chain_id_len + nonce_len + gas_price_len + gas_limit_len + to_len +
                         value_len + data_len + access_list_len;
  const size_t rlp_len = RlpEncoder::list_length(payload);
  const size_t total = 1 + rlp_len;  // 0x01 prefix + RLP

  // Encode
  types::Bytes result(total);
  result[0] = 0x01;

  RlpEncoder enc(std::span<uint8_t>(result).subspan(1));
  enc.start_list(payload);
  enc.encode(tx.chain_id);
  enc.encode(tx.nonce);
  enc.encode(tx.gas_price);
  enc.encode(tx.gas_limit);
  if (tx.to) {
    enc.encode_address(*tx.to);
  } else {
    enc.encode(std::span<const uint8_t>{});
  }
  enc.encode(tx.value);
  enc.encode(std::span<const uint8_t>(tx.data));
  rlp_encode_access_list(enc, tx.access_list);

  return crypto::keccak256(std::span<const uint8_t>(result));
}

// =============================================================================
// Eip2930Tx Sender Recovery
// =============================================================================

std::optional<types::Address> recover_sender(const Eip2930Tx& tx, crypto::Secp256k1Context& ctx) {
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

// =============================================================================
// Eip2930Tx JSON Encoding
// =============================================================================

std::string json_encode(const Eip2930Tx& tx) {
  std::ostringstream ss;
  ss << '{';

  json_append_key(ss, "type");
  json_append_string(ss, "0x1");
  ss << ',';

  json_append_key(ss, "chainId");
  json_append_string(ss, hex::encode_uint64(tx.chain_id));
  ss << ',';

  json_append_key(ss, "nonce");
  json_append_string(ss, hex::encode_uint64(tx.nonce));
  ss << ',';

  json_append_key(ss, "gasPrice");
  json_append_string(ss, hex::encode_uint256(tx.gas_price));
  ss << ',';

  json_append_key(ss, "gas");
  json_append_string(ss, hex::encode_uint64(tx.gas_limit));
  ss << ',';

  json_append_key(ss, "to");
  if (tx.to) {
    json_append_string(ss, hex::encode_address(*tx.to));
  } else {
    json_append_null(ss);
  }
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
// Eip2930Tx JSON Decoding
// =============================================================================

JsonDecodeResult<Eip2930Tx> json_decode_eip2930_tx(std::string_view json) {
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

  Eip2930Tx tx;

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

  // gasPrice (required)
  auto gas_price = json_parse_hex_uint256_field(obj.value(), "gasPrice");
  if (!gas_price) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "gasPrice"};
  }
  tx.gas_price = *gas_price;

  // gas (required)
  auto gas = json_parse_hex_uint64_field(obj.value(), "gas");
  if (!gas) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "gas"};
  }
  tx.gas_limit = *gas;

  // to (optional)
  auto to_result = json_parse_optional_address_field(obj.value(), "to");
  if (!to_result) {
    return {.error = JsonDecodeError::InvalidHex, .error_detail = "to"};
  }
  tx.to = *to_result;

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

}  // namespace div0::ethereum
