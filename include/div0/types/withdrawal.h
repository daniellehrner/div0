#ifndef DIV0_TYPES_WITHDRAWAL_H
#define DIV0_TYPES_WITHDRAWAL_H

/// @file withdrawal.h
/// @brief EIP-4895 Withdrawal type for consensus layer to execution layer transfers.

#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stddef.h>
#include <stdint.h>

/// EIP-4895 Withdrawal (Shanghai+).
/// Represents a withdrawal from the consensus layer to the execution layer.
typedef struct {
  uint64_t index;           ///< Monotonically increasing identifier issued by consensus layer
  uint64_t validator_index; ///< Index of validator associated with withdrawal
  address_t address;        ///< Target address for withdrawn ether
  uint64_t amount;          ///< Value of withdrawal in Gwei (NOT Wei)
} withdrawal_t;

/// Convert withdrawal amount from Gwei to Wei.
/// @param w Withdrawal to convert
/// @return Amount in Wei (amount * 10^9)
static inline uint256_t withdrawal_amount_wei(const withdrawal_t *w) {
  // 1 Gwei = 10^9 Wei
  return uint256_mul(uint256_from_u64(w->amount), uint256_from_u64(1000000000ULL));
}

#ifndef DIV0_FREESTANDING

#include "div0/mem/arena.h"
#include "div0/types/bytes.h"

/// RLP encode a withdrawal as [index, validator_index, address, amount].
/// @param arena Arena for allocation
/// @param w Withdrawal to encode
/// @return Arena-backed bytes_t containing RLP-encoded withdrawal
[[nodiscard]] bytes_t rlp_encode_withdrawal(div0_arena_t *arena, const withdrawal_t *w);

/// Compute the withdrawals trie root from a list of withdrawals.
/// Uses MPT with key = RLP(index in list), value = RLP(withdrawal).
/// @param withdrawals Array of withdrawals
/// @param count Number of withdrawals
/// @param arena Arena for temporary allocations
/// @return Withdrawals root hash (MPT_EMPTY_ROOT if count is 0)
[[nodiscard]] hash_t compute_withdrawals_root(const withdrawal_t *withdrawals, size_t count,
                                              div0_arena_t *arena);

#endif // DIV0_FREESTANDING

#endif // DIV0_TYPES_WITHDRAWAL_H
