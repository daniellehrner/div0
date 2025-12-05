#include <algorithm>

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
// LegacyTx RLP Encoding
// =============================================================================

types::Bytes rlp_encode(const LegacyTx& tx) {
  // RLP: [nonce, gas_price, gas_limit, to, value, data, v, r, s]

  // Phase 1: Calculate lengths
  const size_t nonce_len = RlpEncoder::encoded_length(tx.nonce);
  const size_t gas_price_len = RlpEncoder::encoded_length(tx.gas_price);
  const size_t gas_limit_len = RlpEncoder::encoded_length(tx.gas_limit);
  const size_t to_len = tx.to ? RlpEncoder::encoded_length_address()
                              : RlpEncoder::encoded_length(std::span<const uint8_t>{});
  const size_t value_len = RlpEncoder::encoded_length(tx.value);
  const size_t data_len = RlpEncoder::encoded_length(std::span<const uint8_t>(tx.data));
  const size_t v_len = RlpEncoder::encoded_length(tx.v);
  const size_t r_len = RlpEncoder::encoded_length(tx.r);
  const size_t s_len = RlpEncoder::encoded_length(tx.s);

  const size_t payload = nonce_len + gas_price_len + gas_limit_len + to_len + value_len + data_len +
                         v_len + r_len + s_len;
  const size_t total = RlpEncoder::list_length(payload);

  // Phase 2: Encode
  types::Bytes result(total);
  RlpEncoder enc(result);
  enc.start_list(payload);
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
  enc.encode(tx.v);
  enc.encode(tx.r);
  enc.encode(tx.s);

  return result;
}

// =============================================================================
// LegacyTx RLP Decoding
// =============================================================================

TxDecodeResult<LegacyTx> rlp_decode_legacy_tx(std::span<const uint8_t> data) {
  if (data.empty()) {
    return {.error = TxDecodeError::EmptyInput};
  }

  RlpDecoder dec(data);

  // Decode list header
  auto list_header = dec.decode_list_header();
  if (!list_header.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }

  LegacyTx tx;

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

  // to (optional - empty for contract creation)
  auto to_bytes = dec.decode_bytes();
  if (!to_bytes.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  if (to_bytes.value.empty()) {
    tx.to = std::nullopt;
  } else if (to_bytes.value.size() == 20) {
    std::array<uint8_t, 20> addr_bytes{};
    std::ranges::copy(to_bytes.value, addr_bytes.begin());
    tx.to = types::Address(addr_bytes);
  } else {
    return {.error = TxDecodeError::InvalidRlp};
  }

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

  // v
  auto v_result = dec.decode_uint64();
  if (!v_result.ok()) {
    return {.error = TxDecodeError::InvalidRlp};
  }
  tx.v = v_result.value;

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
// LegacyTx Transaction Hash
// =============================================================================

types::Hash tx_hash(const LegacyTx& tx) {
  if (tx.cached_hash) {
    return *tx.cached_hash;
  }

  auto encoded = rlp_encode(tx);
  auto hash = crypto::keccak256(std::span<const uint8_t>(encoded));
  tx.cached_hash = hash;
  return hash;
}

// =============================================================================
// LegacyTx Signing Hash (for sender recovery)
// =============================================================================

types::Hash signing_hash(const LegacyTx& tx) {
  // For pre-EIP-155 (v = 27 or 28):
  //   keccak256(rlp([nonce, gas_price, gas_limit, to, value, data]))
  //
  // For EIP-155 (v >= 37):
  //   keccak256(rlp([nonce, gas_price, gas_limit, to, value, data, chain_id, 0, 0]))

  const auto chain_id = tx.chain_id();

  // Calculate payload length
  const size_t nonce_len = RlpEncoder::encoded_length(tx.nonce);
  const size_t gas_price_len = RlpEncoder::encoded_length(tx.gas_price);
  const size_t gas_limit_len = RlpEncoder::encoded_length(tx.gas_limit);
  const size_t to_len = tx.to ? RlpEncoder::encoded_length_address()
                              : RlpEncoder::encoded_length(std::span<const uint8_t>{});
  const size_t value_len = RlpEncoder::encoded_length(tx.value);
  const size_t data_len = RlpEncoder::encoded_length(std::span<const uint8_t>(tx.data));

  size_t payload = nonce_len + gas_price_len + gas_limit_len + to_len + value_len + data_len;

  // For EIP-155, append [chain_id, 0, 0]
  if (chain_id) {
    const size_t chain_id_len = RlpEncoder::encoded_length(*chain_id);
    const size_t zero_len = RlpEncoder::encoded_length(uint64_t{0});
    payload += chain_id_len + zero_len + zero_len;
  }

  const size_t total = RlpEncoder::list_length(payload);

  // Encode
  types::Bytes result(total);
  RlpEncoder enc(result);
  enc.start_list(payload);
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

  if (chain_id) {
    enc.encode(*chain_id);
    enc.encode(uint64_t{0});
    enc.encode(uint64_t{0});
  }

  return crypto::keccak256(std::span<const uint8_t>(result));
}

// =============================================================================
// LegacyTx Sender Recovery
// =============================================================================

std::optional<types::Address> recover_sender(const LegacyTx& tx, crypto::Secp256k1Context& ctx) {
  if (tx.cached_sender) {
    return tx.cached_sender;
  }

  auto hash = signing_hash(tx);
  auto sender = ctx.ecrecover(hash.to_uint256(), tx.v, tx.r, tx.s, tx.chain_id());

  if (sender) {
    tx.cached_sender = *sender;
  }

  return sender;
}

// =============================================================================
// LegacyTx JSON Encoding
// =============================================================================

std::string json_encode(const LegacyTx& tx) {
  std::ostringstream ss;
  ss << '{';

  json_append_key(ss, "type");
  json_append_string(ss, "0x0");
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

  json_append_key(ss, "v");
  json_append_string(ss, hex::encode_uint64(tx.v));
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
// LegacyTx JSON Decoding
// =============================================================================

JsonDecodeResult<LegacyTx> json_decode_legacy_tx(std::string_view json) {
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

  LegacyTx tx;

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

  // to (optional - null for contract creation)
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

  // input/data (required, may be empty)
  auto input = json_parse_hex_bytes_field(obj.value(), "input");
  if (!input) {
    // Try "data" field as alternative
    input = json_parse_hex_bytes_field(obj.value(), "data");
    if (!input) {
      return {.error = JsonDecodeError::MissingField, .error_detail = "input"};
    }
  }
  tx.data = std::move(*input);

  // v (required)
  auto v = json_parse_hex_uint64_field(obj.value(), "v");
  if (!v) {
    return {.error = JsonDecodeError::MissingField, .error_detail = "v"};
  }
  tx.v = *v;

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
