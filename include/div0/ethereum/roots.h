#ifndef DIV0_ETHEREUM_ROOTS_H
#define DIV0_ETHEREUM_ROOTS_H

#include <map>
#include <vector>

#include "div0/ethereum/account.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"

namespace div0::ethereum {

/**
 * @brief Compute the state root from a map of accounts.
 *
 * Keys in the trie are keccak256(address), values are RLP-encoded accounts.
 * Uses a Merkle Patricia Trie to compute the root hash.
 *
 * @param accounts Map of address to account state
 * @return State root hash (EMPTY_ROOT if accounts is empty)
 */
[[nodiscard]] types::Hash compute_state_root(
    const std::map<types::Address, AccountState>& accounts);

/**
 * @brief Compute the transactions root from RLP-encoded transactions.
 *
 * Keys in the trie are RLP(index), values are the already-encoded transactions.
 * Transaction index is 0-based.
 *
 * @param tx_rlps Vector of RLP-encoded transactions (in order)
 * @return Transactions root hash (EMPTY_ROOT if empty)
 */
[[nodiscard]] types::Hash compute_transactions_root(const std::vector<types::Bytes>& tx_rlps);

/**
 * @brief Compute the receipts root from RLP-encoded receipts.
 *
 * Keys in the trie are RLP(index), values are the already-encoded receipts.
 * Receipt index is 0-based.
 *
 * @param receipt_rlps Vector of RLP-encoded receipts (in order)
 * @return Receipts root hash (EMPTY_ROOT if empty)
 */
[[nodiscard]] types::Hash compute_receipts_root(const std::vector<types::Bytes>& receipt_rlps);

/**
 * @brief Compute storage root from a map of slots.
 *
 * Keys in the trie are keccak256(slot), values are RLP(value_trimmed).
 * Zero values are not stored in the trie.
 *
 * @param storage Map of slot -> value
 * @return Storage root hash (EMPTY_ROOT if no non-zero slots)
 */
[[nodiscard]] types::Hash compute_storage_root(
    const std::map<types::Uint256, types::Uint256>& storage);

/// Withdrawal structure (EIP-4895)
struct Withdrawal {
  uint64_t index{0};
  uint64_t validator_index{0};
  types::Address address;
  uint64_t amount{0};  // In Gwei
};

/**
 * @brief Compute the withdrawals root from a vector of withdrawals.
 *
 * Keys in the trie are RLP(index), values are RLP(withdrawal).
 * Withdrawal encoding: [Index, Validator, Address, Amount]
 *
 * @param withdrawals Vector of withdrawals (in order)
 * @return Withdrawals root hash (EMPTY_ROOT if empty)
 */
[[nodiscard]] types::Hash compute_withdrawals_root(const std::vector<Withdrawal>& withdrawals);

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_ROOTS_H
