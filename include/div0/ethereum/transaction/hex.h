#ifndef DIV0_ETHEREUM_TRANSACTION_HEX_H
#define DIV0_ETHEREUM_TRANSACTION_HEX_H

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "div0/ethereum/transaction/types.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::ethereum::hex {

// =============================================================================
// Hex Encoding Functions
// =============================================================================

/**
 * @brief Encode uint64 as "0x" prefixed hex string (quantity encoding).
 * Uses minimal digits (no leading zeros), zero as "0x0".
 */
[[nodiscard]] std::string encode_uint64(uint64_t value);

/**
 * @brief Encode Uint256 as "0x" prefixed hex string.
 * Uses minimal digits (no leading zeros), zero as "0x0".
 */
[[nodiscard]] std::string encode_uint256(const types::Uint256& value);

/**
 * @brief Encode byte array as "0x" prefixed hex string.
 */
[[nodiscard]] std::string encode_bytes(std::span<const uint8_t> bytes);

/**
 * @brief Encode address as "0x" prefixed 40-char lowercase hex.
 */
[[nodiscard]] std::string encode_address(const types::Address& addr);

/**
 * @brief Encode hash as "0x" prefixed 64-char lowercase hex.
 */
[[nodiscard]] std::string encode_hash(const types::Hash& hash);

/**
 * @brief Encode storage slot as "0x" prefixed 64-char lowercase hex.
 * StorageSlot is a wrapper around Uint256, encoded with leading zeros.
 */
[[nodiscard]] std::string encode_storage_slot(const StorageSlot& slot);

// =============================================================================
// Hex Decoding Functions
// =============================================================================

/**
 * @brief Decode "0x" prefixed hex string to uint64.
 * @return nullopt on parse error or overflow
 */
[[nodiscard]] std::optional<uint64_t> decode_uint64(std::string_view hex);

/**
 * @brief Decode "0x" prefixed hex string to Uint256.
 * @return nullopt on parse error
 */
[[nodiscard]] std::optional<types::Uint256> decode_uint256(std::string_view hex);

/**
 * @brief Decode "0x" prefixed hex string to byte vector.
 * @return nullopt on parse error
 */
[[nodiscard]] std::optional<types::Bytes> decode_bytes(std::string_view hex);

/**
 * @brief Decode "0x" prefixed 40-char hex string to address.
 * @return nullopt on parse error or wrong length
 */
[[nodiscard]] std::optional<types::Address> decode_address(std::string_view hex);

/**
 * @brief Decode "0x" prefixed 64-char hex string to hash.
 * @return nullopt on parse error or wrong length
 */
[[nodiscard]] std::optional<types::Hash> decode_hash(std::string_view hex);

/**
 * @brief Decode "0x" prefixed 64-char hex string to storage slot.
 * @return nullopt on parse error or wrong length
 */
[[nodiscard]] std::optional<StorageSlot> decode_storage_slot(std::string_view hex);

}  // namespace div0::ethereum::hex

#endif  // DIV0_ETHEREUM_TRANSACTION_HEX_H
