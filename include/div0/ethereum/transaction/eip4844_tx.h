#ifndef DIV0_ETHEREUM_TRANSACTION_EIP4844_TX_H
#define DIV0_ETHEREUM_TRANSACTION_EIP4844_TX_H

#include <cstdint>
#include <optional>
#include <vector>

#include "div0/ethereum/transaction/types.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/bytes32.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief EIP-4844 (Type 3) Blob Transaction.
 *
 * Introduced blob-carrying transactions for rollup data availability.
 * Adds blob versioned hashes and max_fee_per_blob_gas.
 *
 * Envelope: 0x03 || RLP([chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
 *                        gas_limit, to, value, data, access_list, max_fee_per_blob_gas,
 *                        blob_versioned_hashes, y_parity, r, s])
 *
 * Note: Blob transactions cannot create contracts (to is required).
 */
struct Eip4844Tx {
  uint64_t chain_id{1};
  uint64_t nonce{0};
  types::Uint256 max_priority_fee_per_gas;
  types::Uint256 max_fee_per_gas;
  uint64_t gas_limit{0};
  types::Address to;  // Required - blob txs cannot create contracts
  types::Uint256 value;
  types::Bytes data;
  AccessList access_list;

  // Blob-specific fields
  types::Uint256 max_fee_per_blob_gas;
  std::vector<types::Bytes32> blob_versioned_hashes;  // Must not be empty

  // Signature (EIP-2718 uses y_parity)
  uint8_t y_parity{0};  // 0 or 1
  types::Uint256 r;
  types::Uint256 s;

  // Cached values (mutable for lazy computation)
  mutable std::optional<types::Hash> cached_hash_;
  mutable std::optional<types::Address> cached_sender_;

  /**
   * @brief Calculate effective gas price given block base fee.
   *
   * effective_gas_price = min(max_fee_per_gas, base_fee + max_priority_fee_per_gas)
   *
   * @param base_fee Block's base fee per gas
   * @return Effective gas price for this transaction
   */
  [[nodiscard]] types::Uint256 effective_gas_price(const types::Uint256& base_fee) const noexcept {
    const types::Uint256 priority_fee_ceiling = base_fee + max_priority_fee_per_gas;
    return (max_fee_per_gas < priority_fee_ceiling) ? max_fee_per_gas : priority_fee_ceiling;
  }

  /**
   * @brief Get total blob gas used by this transaction.
   * @return blob_count * GAS_PER_BLOB (131072)
   */
  [[nodiscard]] uint64_t blob_gas_used() const noexcept {
    constexpr uint64_t kGasPerBlob = 131072;  // 2^17
    return static_cast<uint64_t>(blob_versioned_hashes.size()) * kGasPerBlob;
  }

  /**
   * @brief Get the recovery id for ECDSA signature recovery.
   * @return 0 or 1 for y-parity
   */
  [[nodiscard]] int recovery_id() const noexcept { return static_cast<int>(y_parity); }

  /**
   * @brief Blob transactions cannot create contracts.
   */
  [[nodiscard]] static constexpr bool is_contract_creation() noexcept { return false; }

  [[nodiscard]] bool operator==(const Eip4844Tx& other) const noexcept {
    return chain_id == other.chain_id && nonce == other.nonce &&
           max_priority_fee_per_gas == other.max_priority_fee_per_gas &&
           max_fee_per_gas == other.max_fee_per_gas && gas_limit == other.gas_limit &&
           to == other.to && value == other.value && data == other.data &&
           access_list == other.access_list && max_fee_per_blob_gas == other.max_fee_per_blob_gas &&
           blob_versioned_hashes == other.blob_versioned_hashes && y_parity == other.y_parity &&
           r == other.r && s == other.s;
  }
};

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_TRANSACTION_EIP4844_TX_H
