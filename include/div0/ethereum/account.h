#ifndef DIV0_ETHEREUM_ACCOUNT_H
#define DIV0_ETHEREUM_ACCOUNT_H

#include <cstdint>

#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::ethereum {

/// Empty code hash: keccak256("")
/// = 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
extern const types::Hash EMPTY_CODE_HASH;

/**
 * @brief Ethereum account state.
 *
 * Represents the state of an account as stored in the state trie.
 * RLP encoding follows Yellow Paper: [nonce, balance, storageRoot, codeHash]
 */
struct AccountState {
  uint64_t nonce{0};
  types::Uint256 balance;
  types::Hash storage_root;  // EMPTY_TRIE_ROOT if no storage
  types::Hash code_hash;     // EMPTY_CODE_HASH if no code
};

/**
 * @brief RLP encode an account for the state trie.
 *
 * Encodes as: [nonce, balance, storage_root, code_hash]
 * where balance is encoded as a variable-length big-endian integer.
 *
 * @param account Account state to encode
 * @return RLP-encoded account
 */
[[nodiscard]] types::Bytes rlp_encode_account(const AccountState& account);

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_ACCOUNT_H
