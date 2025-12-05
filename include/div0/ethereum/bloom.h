#ifndef DIV0_ETHEREUM_BLOOM_H
#define DIV0_ETHEREUM_BLOOM_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::ethereum {

/**
 * @brief Ethereum logs bloom filter (2048 bits = 256 bytes).
 *
 * Used in receipts and block headers to enable quick filtering
 * of logs by address and topics without scanning all logs.
 *
 * For each item added, 3 bits are set based on the keccak-256 hash:
 * - Bits are selected from bytes [0,1], [2,3], and [4,5] of the hash
 * - Each pair gives a bit index: ((b0 << 8) | b1) & 0x7FF
 *
 * This is a probabilistic filter: false positives are possible,
 * but false negatives are not.
 */
class Bloom {
 public:
  static constexpr size_t SIZE = 256;  // 2048 bits = 256 bytes
  static constexpr size_t BIT_COUNT = 2048;

  /// Default constructor: empty bloom filter
  constexpr Bloom() noexcept = default;

  /// Construct from 256 bytes
  explicit Bloom(std::span<const uint8_t, SIZE> bytes) noexcept;

  /// Construct from std::array
  explicit constexpr Bloom(const std::array<uint8_t, SIZE>& bytes) noexcept : data_(bytes) {}

  /// Add an address to the bloom filter
  void add(const types::Address& address) noexcept;

  /// Add a topic (Uint256) to the bloom filter
  void add(const types::Uint256& topic) noexcept;

  /// Add arbitrary data to the bloom filter
  void add(std::span<const uint8_t> data) noexcept;

  /// Check if the bloom filter may contain an address (false positives possible)
  [[nodiscard]] bool may_contain(const types::Address& address) const noexcept;

  /// Check if the bloom filter may contain a topic (false positives possible)
  [[nodiscard]] bool may_contain(const types::Uint256& topic) const noexcept;

  /// Check if the bloom filter may contain data (false positives possible)
  [[nodiscard]] bool may_contain(std::span<const uint8_t> data) const noexcept;

  /// Combine with another bloom filter (bitwise OR)
  Bloom& operator|=(const Bloom& other) noexcept;

  /// Get combined bloom filter
  [[nodiscard]] Bloom operator|(const Bloom& other) const noexcept;

  /// Access raw bytes
  [[nodiscard]] constexpr const uint8_t* data() const noexcept { return data_.data(); }
  [[nodiscard]] constexpr uint8_t* data() noexcept { return data_.data(); }

  /// Access as span
  [[nodiscard]] constexpr std::span<const uint8_t, SIZE> span() const noexcept { return data_; }

  /// Check if empty (all zeros)
  [[nodiscard]] bool is_empty() const noexcept;

  /// Clear the bloom filter
  void clear() noexcept;

  /// Comparison
  [[nodiscard]] bool operator==(const Bloom& other) const noexcept = default;

  /// Create empty bloom filter
  [[nodiscard]] static constexpr Bloom empty() noexcept { return Bloom{}; }

 private:
  std::array<uint8_t, SIZE> data_{};

  /// Set the 3 bits in the bloom filter based on a 32-byte hash
  void set_bits_from_hash(std::span<const uint8_t, 32> hash) noexcept;

  /// Check if the 3 bits from a hash are all set
  [[nodiscard]] bool has_bits_from_hash(std::span<const uint8_t, 32> hash) const noexcept;

  /// Get the bit index from two bytes (big-endian, masked to 11 bits)
  [[nodiscard]] static constexpr uint16_t bit_index(uint8_t high, uint8_t low) noexcept {
    return static_cast<uint16_t>(((high << 8) | low) & 0x7FF);
  }

  /// Set a single bit in the bloom filter
  void set_bit(uint16_t index) noexcept;

  /// Check if a single bit is set
  [[nodiscard]] bool has_bit(uint16_t index) const noexcept;
};

}  // namespace div0::ethereum

#endif  // DIV0_ETHEREUM_BLOOM_H
