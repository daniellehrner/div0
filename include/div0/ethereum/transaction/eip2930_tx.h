#ifndef DIV0_ETHEREUM_TRANSACTION_EIP2930_TX_H
#define DIV0_ETHEREUM_TRANSACTION_EIP2930_TX_H

#include <cstdint>
#include <optional>

#include "div0/ethereum/transaction/types.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief EIP-2930 (Type 1) Access List Transaction.
 *
 * Introduced optional access lists with fixed chain_id.
 * Uses y_parity (0/1) instead of legacy v (27/28).
 *
 * Envelope: 0x01 || RLP([chain_id, nonce, gas_price, gas_limit, to, value, data,
 *                        access_list, y_parity, r, s])
 */
struct Eip2930Tx {
  uint64_t chain_id{1};
  uint64_t nonce{0};
  types::Uint256 gas_price;
  uint64_t gas_limit{0};
  std::optional<types::Address> to;  // nullopt = contract creation
  types::Uint256 value;
  types::Bytes data;
  AccessList access_list;

  // Signature (EIP-2718 uses y_parity)
  uint8_t y_parity{0};  // 0 or 1
  types::Uint256 r;
  types::Uint256 s;

  // Cached values (mutable for lazy computation)
  mutable std::optional<types::Hash> cached_hash_;
  mutable std::optional<types::Address> cached_sender_;

  /**
   * @brief Get the recovery id for ECDSA signature recovery.
   * @return 0 or 1 for y-parity
   */
  [[nodiscard]] int recovery_id() const noexcept { return static_cast<int>(y_parity); }

  /**
   * @brief Check if this is a contract creation transaction.
   */
  [[nodiscard]] bool is_contract_creation() const noexcept { return !to.has_value(); }

  [[nodiscard]] bool operator==(const Eip2930Tx& other) const noexcept {
    return chain_id == other.chain_id && nonce == other.nonce && gas_price == other.gas_price &&
           gas_limit == other.gas_limit && to == other.to && value == other.value &&
           data == other.data && access_list == other.access_list && y_parity == other.y_parity &&
           r == other.r && s == other.s;
  }
};

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_TRANSACTION_EIP2930_TX_H
