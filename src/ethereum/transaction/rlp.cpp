#include "div0/ethereum/transaction/rlp.h"

#include <algorithm>

#include "div0/rlp/decode.h"
#include "div0/rlp/encode.h"

namespace div0::ethereum {

using rlp::RlpDecoder;
using rlp::RlpEncoder;

// =============================================================================
// AccessList RLP Helpers
// =============================================================================

size_t rlp_encoded_length(const AccessList& list) {
  // access_list: [[address, [storage_key, ...]], ...]
  size_t entries_payload = 0;

  for (const auto& entry : list) {
    // Each entry: [address, [keys...]]
    const size_t addr_len = RlpEncoder::encoded_length_address();

    // Keys list payload
    size_t keys_payload = 0;
    for (const auto& key : entry.storage_keys) {
      keys_payload += RlpEncoder::encoded_length(key.get());
    }
    const size_t keys_list_len = RlpEncoder::list_length(keys_payload);

    // Entry list: [address, keys_list]
    const size_t entry_payload = addr_len + keys_list_len;
    entries_payload += RlpEncoder::list_length(entry_payload);
  }

  return RlpEncoder::list_length(entries_payload);
}

void rlp_encode_access_list(RlpEncoder& enc, const AccessList& list) {
  // Calculate outer list payload
  size_t entries_payload = 0;
  for (const auto& entry : list) {
    const size_t addr_len = RlpEncoder::encoded_length_address();
    size_t keys_payload = 0;
    for (const auto& key : entry.storage_keys) {
      keys_payload += RlpEncoder::encoded_length(key.get());
    }
    const size_t keys_list_len = RlpEncoder::list_length(keys_payload);
    const size_t entry_payload = addr_len + keys_list_len;
    entries_payload += RlpEncoder::list_length(entry_payload);
  }

  enc.start_list(entries_payload);

  for (const auto& entry : list) {
    const size_t addr_len = RlpEncoder::encoded_length_address();
    size_t keys_payload = 0;
    for (const auto& key : entry.storage_keys) {
      keys_payload += RlpEncoder::encoded_length(key.get());
    }
    const size_t keys_list_len = RlpEncoder::list_length(keys_payload);
    const size_t entry_payload = addr_len + keys_list_len;

    enc.start_list(entry_payload);
    enc.encode_address(entry.address);
    enc.start_list(keys_payload);
    for (const auto& key : entry.storage_keys) {
      enc.encode(key.get());
    }
  }
}

TxDecodeError rlp_decode_access_list(RlpDecoder& dec, AccessList& list) {
  auto list_header = dec.decode_list_header();
  if (!list_header.ok()) {
    return TxDecodeError::InvalidRlp;
  }

  const size_t list_end = dec.position() + list_header.value;

  while (dec.position() < list_end) {
    auto entry_header = dec.decode_list_header();
    if (!entry_header.ok()) {
      return TxDecodeError::InvalidRlp;
    }

    AccessListEntry entry;

    auto addr_result = dec.decode_address();
    if (!addr_result.ok()) {
      return TxDecodeError::InvalidRlp;
    }
    entry.address = addr_result.value;

    auto keys_header = dec.decode_list_header();
    if (!keys_header.ok()) {
      return TxDecodeError::InvalidRlp;
    }

    const size_t keys_end = dec.position() + keys_header.value;
    while (dec.position() < keys_end) {
      auto key_result = dec.decode_uint256();
      if (!key_result.ok()) {
        return TxDecodeError::InvalidRlp;
      }
      entry.storage_keys.emplace_back(key_result.value);
    }

    list.push_back(std::move(entry));
  }

  return TxDecodeError::Success;
}

// =============================================================================
// AuthorizationList RLP Helpers
// =============================================================================

size_t rlp_encoded_length(const AuthorizationList& list) {
  // authorization_list: [[chain_id, address, nonce, y_parity, r, s], ...]
  size_t entries_payload = 0;

  for (const auto& auth : list) {
    // Each auth: [chain_id, address, nonce, y_parity, r, s]
    const size_t chain_id_len = RlpEncoder::encoded_length(auth.chain_id);
    const size_t addr_len = RlpEncoder::encoded_length_address();
    const size_t nonce_len = RlpEncoder::encoded_length(auth.nonce);
    const size_t y_parity_len = RlpEncoder::encoded_length(auth.y_parity);
    const size_t r_len = RlpEncoder::encoded_length(auth.r);
    const size_t s_len = RlpEncoder::encoded_length(auth.s);

    const size_t auth_payload = chain_id_len + addr_len + nonce_len + y_parity_len + r_len + s_len;
    entries_payload += RlpEncoder::list_length(auth_payload);
  }

  return RlpEncoder::list_length(entries_payload);
}

void rlp_encode_authorization_list(RlpEncoder& enc, const AuthorizationList& list) {
  // Calculate outer list payload
  size_t entries_payload = 0;
  for (const auto& auth : list) {
    const size_t chain_id_len = RlpEncoder::encoded_length(auth.chain_id);
    const size_t addr_len = RlpEncoder::encoded_length_address();
    const size_t nonce_len = RlpEncoder::encoded_length(auth.nonce);
    const size_t y_parity_len = RlpEncoder::encoded_length(auth.y_parity);
    const size_t r_len = RlpEncoder::encoded_length(auth.r);
    const size_t s_len = RlpEncoder::encoded_length(auth.s);
    const size_t auth_payload = chain_id_len + addr_len + nonce_len + y_parity_len + r_len + s_len;
    entries_payload += RlpEncoder::list_length(auth_payload);
  }

  enc.start_list(entries_payload);

  for (const auto& auth : list) {
    const size_t chain_id_len = RlpEncoder::encoded_length(auth.chain_id);
    const size_t addr_len = RlpEncoder::encoded_length_address();
    const size_t nonce_len = RlpEncoder::encoded_length(auth.nonce);
    const size_t y_parity_len = RlpEncoder::encoded_length(auth.y_parity);
    const size_t r_len = RlpEncoder::encoded_length(auth.r);
    const size_t s_len = RlpEncoder::encoded_length(auth.s);
    const size_t auth_payload = chain_id_len + addr_len + nonce_len + y_parity_len + r_len + s_len;

    enc.start_list(auth_payload);
    enc.encode(auth.chain_id);
    enc.encode_address(auth.address);
    enc.encode(auth.nonce);
    enc.encode(auth.y_parity);
    enc.encode(auth.r);
    enc.encode(auth.s);
  }
}

TxDecodeError rlp_decode_authorization_list(RlpDecoder& dec, AuthorizationList& list) {
  auto list_header = dec.decode_list_header();
  if (!list_header.ok()) {
    return TxDecodeError::InvalidRlp;
  }

  const size_t list_end = dec.position() + list_header.value;

  while (dec.position() < list_end) {
    // Decode auth: [chain_id, address, nonce, y_parity, r, s]
    auto auth_header = dec.decode_list_header();
    if (!auth_header.ok()) {
      return TxDecodeError::InvalidRlp;
    }

    Authorization auth;

    auto chain_id_result = dec.decode_uint64();
    if (!chain_id_result.ok()) {
      return TxDecodeError::InvalidRlp;
    }
    auth.chain_id = chain_id_result.value;

    auto addr_result = dec.decode_address();
    if (!addr_result.ok()) {
      return TxDecodeError::InvalidRlp;
    }
    auth.address = addr_result.value;

    auto nonce_result = dec.decode_uint64();
    if (!nonce_result.ok()) {
      return TxDecodeError::InvalidRlp;
    }
    auth.nonce = nonce_result.value;

    auto y_parity_result = dec.decode_uint64();
    if (!y_parity_result.ok()) {
      return TxDecodeError::InvalidRlp;
    }
    auth.y_parity = static_cast<uint8_t>(y_parity_result.value);

    auto r_result = dec.decode_uint256();
    if (!r_result.ok()) {
      return TxDecodeError::InvalidRlp;
    }
    auth.r = r_result.value;

    auto s_result = dec.decode_uint256();
    if (!s_result.ok()) {
      return TxDecodeError::InvalidRlp;
    }
    auth.s = s_result.value;

    list.push_back(auth);
  }

  return TxDecodeError::Success;
}

// =============================================================================
// Optional Address Decoding Helper
// =============================================================================

std::optional<std::optional<types::Address>> rlp_decode_optional_address(RlpDecoder& dec) {
  auto to_bytes = dec.decode_bytes();
  if (!to_bytes.ok()) {
    return std::nullopt;  // Decode error
  }
  if (to_bytes.value.empty()) {
    return std::optional<types::Address>{std::nullopt};  // Contract creation
  }
  if (to_bytes.value.size() == 20) {
    std::array<uint8_t, 20> addr_bytes{};
    std::ranges::copy(to_bytes.value, addr_bytes.begin());
    return std::optional{types::Address(addr_bytes)};
  }
  return std::nullopt;  // Invalid length
}

// =============================================================================
// Unified Transaction RLP Encoding
// =============================================================================

types::Bytes rlp_encode(const Transaction& tx) {
  return std::visit([](const auto& tx_data) -> types::Bytes { return rlp_encode(tx_data); },
                    tx.variant());
}

}  // namespace div0::ethereum
