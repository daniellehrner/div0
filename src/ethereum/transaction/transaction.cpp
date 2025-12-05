#include "div0/crypto/secp256k1.h"
#include "div0/ethereum/transaction/rlp.h"

namespace div0::ethereum {

// =============================================================================
// Transaction RLP Decoding with Envelope Detection
// =============================================================================

TxDecodeResult<Transaction> rlp_decode_transaction(const std::span<const uint8_t> data) {
  if (data.empty()) {
    return {.error = TxDecodeError::EmptyInput};
  }

  const uint8_t first = data[0];

  // Envelope detection:
  // - If first byte >= 0xc0, it's a legacy tx (starts with RLP list prefix)
  // - If first byte is 0x01-0x04, it's a typed tx (EIP-2718)
  // - Values 0x05-0x7f are reserved for future types
  // - Values 0x80-0xbf are invalid (RLP string prefixes)

  if (first >= 0xc0) {
    // Legacy transaction - first byte is list prefix
    auto result = rlp_decode_legacy_tx(data);
    if (!result.ok()) {
      return {.error = result.error};
    }
    return {.value = Transaction(std::move(result.value))};
  }

  if (first > 0x04) {
    // Invalid type byte - either reserved future type or invalid RLP
    return {.error = TxDecodeError::InvalidType};
  }

  // Typed transaction - strip type byte and decode payload
  auto payload = data.subspan(1);

  switch (first) {
    case 0x01: {
      // EIP-2930 Access List Transaction
      auto result = rlp_decode_eip2930_tx(payload);
      if (!result.ok()) {
        return {.error = result.error};
      }
      return {.value = Transaction(std::move(result.value))};
    }
    case 0x02: {
      // EIP-1559 Dynamic Fee Transaction
      auto result = rlp_decode_eip1559_tx(payload);
      if (!result.ok()) {
        return {.error = result.error};
      }
      return {.value = Transaction(std::move(result.value))};
    }
    case 0x03: {
      // EIP-4844 Blob Transaction
      auto result = rlp_decode_eip4844_tx(payload);
      if (!result.ok()) {
        return {.error = result.error};
      }
      return {.value = Transaction(std::move(result.value))};
    }
    case 0x04: {
      // EIP-7702 Set Code Transaction
      auto result = rlp_decode_eip7702_tx(payload);
      if (!result.ok()) {
        return {.error = result.error};
      }
      return {.value = Transaction(std::move(result.value))};
    }
    default:
      // Type 0x00 is not a valid typed transaction prefix
      return {.error = TxDecodeError::InvalidType};
  }
}

// =============================================================================
// Transaction Hash (dispatches to type-specific implementations)
// =============================================================================

types::Hash tx_hash(const Transaction& tx) {
  return std::visit([](const auto& t) { return tx_hash(t); }, tx.variant());
}

// =============================================================================
// Transaction Signing Hash (dispatches to type-specific implementations)
// =============================================================================

types::Hash signing_hash(const Transaction& tx) {
  return std::visit([](const auto& t) { return signing_hash(t); }, tx.variant());
}

// =============================================================================
// Transaction Sender Recovery (dispatches to type-specific implementations)
// =============================================================================

std::optional<types::Address> recover_sender(const Transaction& tx, crypto::Secp256k1Context& ctx) {
  return std::visit([&ctx](const auto& t) { return recover_sender(t, ctx); }, tx.variant());
}

}  // namespace div0::ethereum
