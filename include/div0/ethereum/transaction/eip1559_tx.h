#ifndef DIV0_ETHEREUM_TRANSACTION_EIP1559_TX_H
#define DIV0_ETHEREUM_TRANSACTION_EIP1559_TX_H

#include <cstdint>
#include <optional>

#include "div0/ethereum/transaction/types.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief EIP-1559 (Type 2) Dynamic Fee Transaction.
 *
 * Introduced base fee + priority fee model for more predictable gas prices.
 * Replaces gas_price with max_fee_per_gas and max_priority_fee_per_gas.
 *
 * Envelope: 0x02 || RLP([chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas,
 *                        gas_limit, to, value, data, access_list, y_parity, r, s])
 */
struct Eip1559Tx {
  // Ordered for optimal memory layout (minimize padding)
  types::Uint256 max_priority_fee_per_gas;
  types::Uint256 max_fee_per_gas;
  types::Uint256 value;
  types::Uint256 r;
  types::Uint256 s;
  mutable std::optional<types::Hash> cached_hash;
  uint64_t chain_id{1};
  uint64_t nonce{0};
  uint64_t gas_limit{0};
  types::Bytes data;
  AccessList access_list;
  uint8_t y_parity{0};               // 0 or 1
  std::optional<types::Address> to;  // nullopt = contract creation
  mutable std::optional<types::Address> cached_sender;

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
   * @brief Check if this is a contract creation transaction.
   */
  [[nodiscard]] bool is_contract_creation() const noexcept { return !to.has_value(); }

  [[nodiscard]] bool operator==(const Eip1559Tx& other) const noexcept {
    return chain_id == other.chain_id && nonce == other.nonce &&
           max_priority_fee_per_gas == other.max_priority_fee_per_gas &&
           max_fee_per_gas == other.max_fee_per_gas && gas_limit == other.gas_limit &&
           to == other.to && value == other.value && data == other.data &&
           access_list == other.access_list && y_parity == other.y_parity && r == other.r &&
           s == other.s;
  }
};

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_TRANSACTION_EIP1559_TX_H
