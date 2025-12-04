#include "div0/ethereum/roots.h"

#include "div0/crypto/keccak256.h"
#include "div0/rlp/encode.h"
#include "div0/trie/mpt.h"

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

}  // namespace div0::ethereum
