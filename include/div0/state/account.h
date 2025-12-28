#ifndef DIV0_STATE_ACCOUNT_H
#define DIV0_STATE_ACCOUNT_H

#include "div0/mem/arena.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Empty code hash: keccak256("").
/// = 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
extern const hash_t EMPTY_CODE_HASH;

/// Ethereum account state.
/// Stored in state trie as RLP([nonce, balance, storage_root, code_hash]).
typedef struct {
  uint64_t nonce;      // Transaction count / contract creation nonce
  uint256_t balance;   // Wei balance
  hash_t storage_root; // Root of account's storage trie
  hash_t code_hash;    // keccak256(code) or EMPTY_CODE_HASH
} account_t;

/// Create an empty account (zero nonce, zero balance, empty storage, no code).
[[nodiscard]] account_t account_empty(void);

/// Check if account is empty (EIP-161: no code, zero nonce, zero balance).
/// An account is considered empty if it has:
/// - nonce == 0
/// - balance == 0
/// - code_hash == EMPTY_CODE_HASH
/// Empty accounts are not stored in the state trie (EIP-161).
[[nodiscard]] bool account_is_empty(const account_t *acc);

/// RLP-encode an account for state trie storage.
/// Format: RLP([nonce, balance, storage_root, code_hash])
/// @param acc Account to encode
/// @param arena Arena for allocation
/// @return RLP-encoded bytes
[[nodiscard]] bytes_t account_rlp_encode(const account_t *acc, div0_arena_t *arena);

/// RLP-decode an account from state trie data.
/// Expects format: RLP([nonce, balance, storage_root, code_hash])
/// @param data RLP-encoded account data
/// @param len Length of data
/// @param out Output account (populated on success)
/// @return true on success, false on decode error
[[nodiscard]] bool account_rlp_decode(const uint8_t *data, size_t len, account_t *out);

#endif // DIV0_STATE_ACCOUNT_H
