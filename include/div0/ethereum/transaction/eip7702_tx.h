#ifndef DIV0_ETHEREUM_TRANSACTION_EIP7702_TX_H
#define DIV0_ETHEREUM_TRANSACTION_EIP7702_TX_H

#include <cstdint>
#include <optional>

#include "div0/ethereum/transaction/types.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief EIP-7702 (Type 4) Set Code Transaction.
 *
 * Allows EOAs to temporarily delegate to contract code via authorization tuples.
 * Each authorization is signed by an EOA authorizing code execution.
 *
 * Envelope: 0x04 || RLP([chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
 *                        gas_limit, to, value, data, access_list, authorization_list,
 *                        y_parity, r, s])
 *
 * Authorization signature: keccak(0x05 || RLP([chain_id, address, nonce]))
 *
 * Note: Set Code transactions cannot create contracts (to is required).
 * Note: authorization_list must not be empty.
 *
 * Gas costs:
 * - PER_AUTH_BASE_COST: 12500 per authorization
 * - PER_EMPTY_ACCOUNT_COST: 25000 if authorizing account doesn't exist
 */
struct Eip7702Tx {
  uint64_t chain_id{1};
  uint64_t nonce{0};
  types::Uint256 max_priority_fee_per_gas;
  types::Uint256 max_fee_per_gas;
  uint64_t gas_limit{0};
  types::Address to;  // Required - cannot create contracts
  types::Uint256 value;
  types::Bytes data;
  AccessList access_list;
  AuthorizationList authorization_list;  // Must not be empty

  // Signature (EIP-2718 uses y_parity)
  uint8_t y_parity{0};  // 0 or 1
  types::Uint256 r;
  types::Uint256 s;

  // Cached values (mutable for lazy computation)
  mutable std::optional<types::Hash> cached_hash_;
  mutable std::optional<types::Address> cached_sender_;

  // EIP-7702 gas constants
  static constexpr uint64_t PER_AUTH_BASE_COST = 12500;
  static constexpr uint64_t PER_EMPTY_ACCOUNT_COST = 25000;

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
   * @brief Get the recovery id for ECDSA signature recovery.
   * @return 0 or 1 for y-parity
   */
  [[nodiscard]] int recovery_id() const noexcept { return static_cast<int>(y_parity); }

  /**
   * @brief Set Code transactions cannot create contracts.
   */
  [[nodiscard]] static constexpr bool is_contract_creation() noexcept { return false; }

  [[nodiscard]] bool operator==(const Eip7702Tx& other) const noexcept {
    return chain_id == other.chain_id && nonce == other.nonce &&
           max_priority_fee_per_gas == other.max_priority_fee_per_gas &&
           max_fee_per_gas == other.max_fee_per_gas && gas_limit == other.gas_limit &&
           to == other.to && value == other.value && data == other.data &&
           access_list == other.access_list && authorization_list == other.authorization_list &&
           y_parity == other.y_parity && r == other.r && s == other.s;
  }
};

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_TRANSACTION_EIP7702_TX_H
