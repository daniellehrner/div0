#ifndef DIV0_ETHEREUM_TRANSACTION_LEGACY_TX_H
#define DIV0_ETHEREUM_TRANSACTION_LEGACY_TX_H

#include <cstdint>
#include <optional>

#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief Legacy (Type 0) Ethereum transaction.
 *
 * Pre-EIP-2718 transaction format with simple gas price.
 * Signature uses v value (27/28 or EIP-155 encoded with chain_id).
 *
 * RLP encoding: [nonce, gas_price, gas_limit, to, value, data, v, r, s]
 */
struct LegacyTx {
  // Ordered for optimal memory layout (minimize padding)
  types::Uint256 gas_price;
  types::Uint256 value;
  types::Uint256 r;
  types::Uint256 s;
  mutable std::optional<types::Hash> cached_hash;
  uint64_t nonce{0};
  uint64_t gas_limit{0};
  uint64_t v{0};  // 27/28 or EIP-155 encoded (chain_id * 2 + 35/36)
  types::Bytes data;
  std::optional<types::Address> to;  // nullopt = contract creation
  mutable std::optional<types::Address> cached_sender;

  /**
   * @brief Extract chain_id from EIP-155 v value.
   * @return Chain ID if EIP-155 encoded, nullopt for pre-EIP-155 (v=27/28)
   */
  [[nodiscard]] std::optional<uint64_t> chain_id() const noexcept {
    if (v == 27 || v == 28) {
      return std::nullopt;  // Pre-EIP-155
    }
    // EIP-155: v = chain_id * 2 + 35 or chain_id * 2 + 36
    return (v - 35) / 2;
  }

  /**
   * @brief Get the recovery id for ECDSA signature recovery.
   * @return 0 or 1 for y-parity
   */
  [[nodiscard]] int recovery_id() const noexcept {
    if (v == 27 || v == 28) {
      return static_cast<int>(v - 27);
    }
    // EIP-155: v = chain_id * 2 + 35 + y_parity
    return static_cast<int>((v - 35) % 2);
  }

  /**
   * @brief Check if this is a contract creation transaction.
   */
  [[nodiscard]] bool is_contract_creation() const noexcept { return !to.has_value(); }

  [[nodiscard]] bool operator==(const LegacyTx& other) const noexcept {
    return nonce == other.nonce && gas_price == other.gas_price && gas_limit == other.gas_limit &&
           to == other.to && value == other.value && data == other.data && v == other.v &&
           r == other.r && s == other.s;
  }
};

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_TRANSACTION_LEGACY_TX_H
