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

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_ROOTS_H
