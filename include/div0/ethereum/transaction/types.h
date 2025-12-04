#ifndef DIV0_ETHEREUM_TRANSACTION_TYPES_H
#define DIV0_ETHEREUM_TRANSACTION_TYPES_H

#include <cstdint>
#include <optional>
#include <vector>

#include "div0/ethereum/storage_slot.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief EIP-2718 transaction type identifiers.
 */
enum class TxType : uint8_t {
  Legacy = 0,      // Pre-EIP-2718 (no type prefix)
  AccessList = 1,  // EIP-2930
  DynamicFee = 2,  // EIP-1559
  Blob = 3,        // EIP-4844
  SetCode = 4,     // EIP-7702
};

/**
 * @brief Access list entry (EIP-2930).
 *
 * Specifies an address and its storage slots that will be accessed.
 */
struct AccessListEntry {
  types::Address address;
  std::vector<StorageSlot> storage_keys;

  [[nodiscard]] bool operator==(const AccessListEntry& other) const noexcept = default;
};

using AccessList = std::vector<AccessListEntry>;

/**
 * @brief EIP-7702 authorization tuple.
 *
 * Allows an EOA to authorize code execution via delegation.
 * Signed by the EOA to authorize setting code.
 *
 * Signature message: keccak(0x05 || rlp([chain_id, address, nonce]))
 */
struct Authorization {
  uint64_t chain_id{0};  // 0 = any chain
  types::Address address;
  uint64_t nonce{0};

  // Signature (y_parity instead of v for EIP-2718 typed txs)
  uint8_t y_parity{0};
  types::Uint256 r;
  types::Uint256 s;

  // Cached recovered authority address
  mutable std::optional<types::Address> cached_authority_;

  [[nodiscard]] bool operator==(const Authorization& other) const noexcept {
    return chain_id == other.chain_id && address == other.address && nonce == other.nonce &&
           y_parity == other.y_parity && r == other.r && s == other.s;
  }
};

using AuthorizationList = std::vector<Authorization>;

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_TRANSACTION_TYPES_H
