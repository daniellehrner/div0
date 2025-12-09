#ifndef DIV0_UTILS_HEX_H
#define DIV0_UTILS_HEX_H

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

namespace div0::hex {

// =============================================================================
// Hex Encoding Functions
// =============================================================================

/**
 * @brief Encode a byte as two lowercase hex characters (no prefix).
 * Returns exactly 2 characters.
 */
[[nodiscard]] std::string encode_byte(uint8_t byte);

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
 * @brief Encode Uint256 as "0x" prefixed 64-char hex string with zero-padding.
 * Always outputs exactly 66 characters (0x + 64 hex digits).
 * Used for topics and other fixed-width 32-byte values.
 */
[[nodiscard]] std::string encode_uint256_padded(const types::Uint256& value);

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

}  // namespace div0::hex

#endif  // DIV0_UTILS_HEX_H
