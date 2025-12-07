#include "div0/ethereum/receipt.h"

#include "div0/rlp/encode.h"

namespace div0::ethereum {

using rlp::RlpEncoder;

// =============================================================================
// Internal Helpers
// =============================================================================

namespace {

size_t rlp_encoded_length_log(const evm::Log& log) {
  // Log: [address, [topic1, topic2, ...], data]
  const size_t addr_len = RlpEncoder::encoded_length_address();

  // Topics list
  size_t topics_payload = 0;
  for (const auto& topic : log.topics) {
    topics_payload += RlpEncoder::encoded_length(topic);
  }
  const size_t topics_list_len = RlpEncoder::list_length(topics_payload);

  // Data
  const size_t data_len = RlpEncoder::encoded_length(std::span(log.data));

  const size_t log_payload = addr_len + topics_list_len + data_len;
  return RlpEncoder::list_length(log_payload);
}

size_t rlp_encoded_length_logs_list(const std::vector<evm::Log>& logs) {
  size_t logs_payload = 0;
  for (const auto& log : logs) {
    logs_payload += rlp_encoded_length_log(log);
  }
  return logs_payload;
}

void encode_log_to(RlpEncoder& enc, const evm::Log& log) {
  // Log: [address, [topic1, topic2, ...], data]
  const size_t addr_len = RlpEncoder::encoded_length_address();

  // Topics list payload
  size_t topics_payload = 0;
  for (const auto& topic : log.topics) {
    topics_payload += RlpEncoder::encoded_length(topic);
  }
  const size_t topics_list_len = RlpEncoder::list_length(topics_payload);

  // Data length
  const size_t data_len = RlpEncoder::encoded_length(std::span(log.data));

  // Total log payload
  const size_t log_payload = addr_len + topics_list_len + data_len;

  enc.start_list(log_payload);
  enc.encode_address(log.address);

  // Encode topics list
  enc.start_list(topics_payload);
  for (const auto& topic : log.topics) {
    enc.encode(topic);
  }

  enc.encode(std::span(log.data));
}

void encode_logs_list_to(RlpEncoder& enc, const std::vector<evm::Log>& logs) {
  // Calculate logs list payload
  const size_t logs_payload = rlp_encoded_length_logs_list(logs);

  enc.start_list(logs_payload);
  for (const auto& log : logs) {
    encode_log_to(enc, log);
  }
}

}  // namespace

// =============================================================================
// Public Log RLP Encoding
// =============================================================================

types::Bytes rlp_encode(const evm::Log& log) {
  const size_t total_len = rlp_encoded_length_log(log);
  types::Bytes result(total_len);
  RlpEncoder enc(result);
  encode_log_to(enc, log);
  return result;
}

types::Bytes rlp_encode_logs(const std::vector<evm::Log>& logs) {
  const size_t logs_payload = rlp_encoded_length_logs_list(logs);
  const size_t total_len = RlpEncoder::list_length(logs_payload);
  types::Bytes result(total_len);
  RlpEncoder enc(result);
  encode_logs_list_to(enc, logs);
  return result;
}

// =============================================================================
// Receipt RLP Encoding
// =============================================================================

types::Bytes rlp_encode(const Receipt& receipt) {
  // Receipt: [status, cumulative_gas_used, bloom, logs]
  // For typed txs: type_byte || RLP([...])

  // Calculate payload length
  const size_t status_len = RlpEncoder::encoded_length(receipt.status);
  const size_t gas_len = RlpEncoder::encoded_length(receipt.cumulative_gas_used);
  const size_t bloom_len = RlpEncoder::encoded_length(receipt.bloom.span());
  const size_t logs_len = RlpEncoder::list_length(rlp_encoded_length_logs_list(receipt.logs));

  const size_t payload = status_len + gas_len + bloom_len + logs_len;
  const size_t rlp_len = RlpEncoder::list_length(payload);

  if (receipt.tx_type == TxType::Legacy) {
    // Legacy receipt: no type prefix
    types::Bytes result(rlp_len);
    RlpEncoder enc(result);
    enc.start_list(payload);
    enc.encode(receipt.status);
    enc.encode(receipt.cumulative_gas_used);
    enc.encode(receipt.bloom.span());
    encode_logs_list_to(enc, receipt.logs);
    return result;
  }

  // Typed receipt: type_byte || RLP([...])
  const size_t total = 1 + rlp_len;
  types::Bytes result(total);
  result[0] = static_cast<uint8_t>(receipt.tx_type);

  RlpEncoder enc(std::span<uint8_t>(result).subspan(1));
  enc.start_list(payload);
  enc.encode(receipt.status);
  enc.encode(receipt.cumulative_gas_used);
  enc.encode(receipt.bloom.span());
  encode_logs_list_to(enc, receipt.logs);

  return result;
}

}  // namespace div0::ethereum
