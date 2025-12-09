#include "div0/ethereum/roots.h"

#include "div0/crypto/keccak256.h"
#include "div0/rlp/encode.h"
#include "div0/trie/mpt.h"
#include "div0/utils/bytes.h"

namespace div0::ethereum {

types::Hash compute_state_root(const std::map<types::Address, AccountState>& accounts) {
  if (accounts.empty()) {
    return trie::MerklePatriciaTrie::EMPTY_ROOT;
  }

  trie::MerklePatriciaTrie trie;

  for (const auto& [address, account] : accounts) {
    // Key: keccak256(address)
    const types::Hash key_hash = crypto::keccak256(address.span());

    // Value: RLP-encoded account
    const types::Bytes value = rlp_encode_account(account);

    // Insert into trie using hash as key
    trie.insert(key_hash.span(), value);
  }

  return trie.root_hash();
}

types::Hash compute_transactions_root(const std::vector<types::Bytes>& tx_rlps) {
  if (tx_rlps.empty()) {
    return trie::MerklePatriciaTrie::EMPTY_ROOT;
  }

  trie::MerklePatriciaTrie trie;

  for (size_t i = 0; i < tx_rlps.size(); ++i) {
    // Key: RLP(index)
    const size_t key_len = rlp::RlpEncoder::encoded_length(static_cast<uint64_t>(i));
    types::Bytes key(key_len);
    rlp::RlpEncoder encoder(key);
    encoder.encode(static_cast<uint64_t>(i));

    // Value: already RLP-encoded transaction
    trie.insert(key, tx_rlps[i]);
  }

  return trie.root_hash();
}

types::Hash compute_receipts_root(const std::vector<types::Bytes>& receipt_rlps) {
  if (receipt_rlps.empty()) {
    return trie::MerklePatriciaTrie::EMPTY_ROOT;
  }

  trie::MerklePatriciaTrie trie;

  for (size_t i = 0; i < receipt_rlps.size(); ++i) {
    // Key: RLP(index)
    const size_t key_len = rlp::RlpEncoder::encoded_length(static_cast<uint64_t>(i));
    types::Bytes key(key_len);
    rlp::RlpEncoder encoder(key);
    encoder.encode(static_cast<uint64_t>(i));

    // Value: already RLP-encoded receipt
    trie.insert(key, receipt_rlps[i]);
  }

  return trie.root_hash();
}

types::Hash compute_storage_root(const std::map<types::Uint256, types::Uint256>& storage) {
  // Count non-zero entries
  bool has_non_zero = false;
  for (const auto& [slot, value] : storage) {
    if (!value.is_zero()) {
      has_non_zero = true;
      break;
    }
  }

  if (!has_non_zero) {
    return trie::MerklePatriciaTrie::EMPTY_ROOT;
  }

  trie::MerklePatriciaTrie trie;

  for (const auto& [slot, value] : storage) {
    if (value.is_zero()) {
      continue;  // Don't store zero values
    }

    // Key: keccak256(slot as 32-byte big-endian)
    const auto slot_be = utils::swap_endian_256(slot.data_unsafe());
    std::array<uint8_t, 32> slot_bytes{};
    std::memcpy(slot_bytes.data(), slot_be.data(), 32);
    const types::Hash key_hash = crypto::keccak256(std::span<const uint8_t, 32>(slot_bytes));

    // Value: RLP(value without leading zeros)
    const size_t val_len = rlp::RlpEncoder::encoded_length(value);
    types::Bytes rlp_value(val_len);
    rlp::RlpEncoder encoder(rlp_value);
    encoder.encode(value);

    trie.insert(key_hash.span(), rlp_value);
  }

  return trie.root_hash();
}

types::Hash compute_withdrawals_root(const std::vector<Withdrawal>& withdrawals) {
  if (withdrawals.empty()) {
    return trie::MerklePatriciaTrie::EMPTY_ROOT;
  }

  trie::MerklePatriciaTrie trie;

  for (size_t i = 0; i < withdrawals.size(); ++i) {
    // Key: RLP(index)
    const size_t key_len = rlp::RlpEncoder::encoded_length(static_cast<uint64_t>(i));
    types::Bytes key(key_len);
    rlp::RlpEncoder key_encoder(key);
    key_encoder.encode(static_cast<uint64_t>(i));

    // Value: RLP([Index, Validator, Address, Amount])
    const auto& w = withdrawals[i];

    // Calculate total length
    const size_t index_len = rlp::RlpEncoder::encoded_length(w.index);
    const size_t validator_len = rlp::RlpEncoder::encoded_length(w.validator_index);
    const size_t address_len = rlp::RlpEncoder::encoded_length(w.address);
    const size_t amount_len = rlp::RlpEncoder::encoded_length(w.amount);
    const size_t inner_len = index_len + validator_len + address_len + amount_len;
    const size_t total_len = rlp::RlpEncoder::list_header_length(inner_len) + inner_len;

    types::Bytes value(total_len);
    rlp::RlpEncoder encoder(value);
    encoder.start_list(inner_len);
    encoder.encode(w.index);
    encoder.encode(w.validator_index);
    encoder.encode(w.address);
    encoder.encode(w.amount);

    trie.insert(key, value);
  }

  return trie.root_hash();
}

}  // namespace div0::ethereum
