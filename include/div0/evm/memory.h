#ifndef DIV0_EVM_MEMORY_H
#define DIV0_EVM_MEMORY_H

#include <cstdint>
#include <span>
#include <vector>

#include "div0/types/uint256.h"

namespace div0::evm {

/**
 * @brief EVM Memory for bytecode execution.
 *
 * Provides byte-addressable storage that expands on-demand according to EVM semantics.
 * Memory is zero-initialized and expands monotonically (never shrinks during execution).
 *
 * ADDRESSING:
 * - Byte-addressable with 32-byte word alignment for MLOAD/MSTORE
 * - Addresses are uint64_t (Uint256 offsets > 2^64 would exceed gas limits)
 * - Unwritten memory reads return 0 (EVM spec)
 *
 * STORAGE MODEL:
 * - size_: highest byte offset accessed + 1, word-aligned (determines gas cost)
 * - data_.size(): actual allocated capacity (may exceed or equal size_)
 *
 * ERROR HANDLING:
 * - Normal operations: noexcept with bounds ensured by caller via expand()
 * - expand(): throws std::bad_alloc on OS memory exhaustion (catastrophic failure)
 *
 * THREAD SAFETY:
 * - NOT thread-safe (each EVM instance has its own Memory)
 */
class Memory {
 public:
  /// Default initial capacity (4KB - typical contract memory usage)
  static constexpr size_t DEFAULT_CAPACITY = 4096;

  /// Word size in bytes (32 bytes = 256 bits)
  static constexpr size_t WORD_SIZE = 32;

  /**
   * @brief Construct memory with optional initial capacity.
   * @param initial_capacity Reserved capacity in bytes (does not affect gas)
   * @throws std::bad_alloc if reservation fails
   */
  Memory(size_t initial_capacity = DEFAULT_CAPACITY);  // NOLINT(google-explicit-constructor)

  // ============================================================================
  // Core Access Operations (UNSAFE - caller must ensure bounds via expand())
  // ============================================================================

  /**
   * @brief Load 32-byte word from memory (MLOAD opcode).
   * @param offset Byte offset
   * @return 32-byte word as Uint256 (big-endian)
   * @warning UNSAFE: Caller must ensure offset + 32 <= size()
   */
  [[nodiscard]] [[gnu::always_inline]]
  types::Uint256 load_word_unsafe(uint64_t offset) const noexcept;

  /**
   * @brief Store 32-byte word to memory (MSTORE opcode).
   * @param offset Byte offset
   * @param value 32-byte word to store (big-endian)
   * @warning UNSAFE: Caller must ensure offset + 32 <= size()
   */
  [[gnu::always_inline]]
  void store_word_unsafe(uint64_t offset, const types::Uint256& value) noexcept;

  /**
   * @brief Store single byte to memory (MSTORE8 opcode).
   * @param offset Byte offset
   * @param value Byte value
   * @warning UNSAFE: Caller must ensure offset < size()
   */
  [[gnu::always_inline]]
  void store_byte_unsafe(uint64_t offset, uint8_t value) noexcept {
    data_[offset] = value;
  }

  /**
   * @brief Get read-only span of memory bytes (for SHA3, RETURN, etc.).
   * @param offset Starting byte offset
   * @param length Number of bytes
   * @return Read-only span of bytes
   * @warning UNSAFE: Caller must ensure offset + length <= size()
   */
  [[nodiscard]] [[gnu::always_inline]]
  std::span<const uint8_t> read_span_unsafe(uint64_t offset, uint64_t length) const noexcept {
    return {&data_[offset], length};
  }

  /**
   * @brief Get writable span of memory bytes (for COPY operations).
   * @param offset Starting byte offset
   * @param length Number of bytes
   * @return Writable span of bytes
   * @warning UNSAFE: Caller must ensure offset + length <= size()
   */
  [[nodiscard]] [[gnu::always_inline]]
  std::span<uint8_t> write_span_unsafe(uint64_t offset, uint64_t length) noexcept {
    return {&data_[offset], length};
  }

  // ============================================================================
  // Memory Expansion
  // ============================================================================

  /**
   * @brief Expand memory to accommodate new_size_bytes.
   *
   * Expands to word-aligned boundary and zero-fills new bytes.
   *
   * @param new_size_bytes Minimum required size in bytes
   * @throws std::bad_alloc if OS cannot allocate memory (catastrophic failure)
   */
  void expand(uint64_t new_size_bytes);

  /**
   * @brief Round byte_size up to nearest 32-byte boundary.
   * @param byte_size Raw byte size
   * @return Word-aligned size (multiple of 32)
   */
  [[nodiscard]] [[gnu::always_inline]]
  static constexpr uint64_t word_aligned_size(uint64_t byte_size) noexcept {
    return ((byte_size + WORD_SIZE - 1) / WORD_SIZE) * WORD_SIZE;
  }

  /**
   * @brief Get number of 32-byte words (rounded up) for byte_size.
   * @param byte_size Size in bytes
   * @return Number of 32-byte words
   */
  [[nodiscard]] [[gnu::always_inline]]
  static constexpr uint64_t word_count(uint64_t byte_size) noexcept {
    return (byte_size + WORD_SIZE - 1) / WORD_SIZE;
  }

  // ============================================================================
  // Size and State Queries
  // ============================================================================

  /**
   * @brief Get current memory size in bytes (word-aligned).
   * @return Current size in bytes (always multiple of 32)
   */
  [[nodiscard]] [[gnu::always_inline]]
  uint64_t size() const noexcept {
    return size_;
  }

  /**
   * @brief Get current memory size in 32-byte words.
   * @return Number of 32-byte words
   */
  [[nodiscard]] [[gnu::always_inline]]
  uint64_t size_words() const noexcept {
    return size_ / WORD_SIZE;
  }

  /**
   * @brief Check if memory is empty.
   * @return true if size is 0
   */
  [[nodiscard]] [[gnu::always_inline]]
  bool empty() const noexcept {
    return size_ == 0;
  }

  /**
   * @brief Reset size to 0 (does not deallocate).
   */
  [[gnu::always_inline]]
  void clear() noexcept {
    size_ = 0;
  }

 private:
  /// Underlying storage (zero-filled on expand)
  std::vector<uint8_t> data_;

  /// Logical size (word-aligned high-water mark for gas calculations)
  uint64_t size_{0};
};

}  // namespace div0::evm

#endif  // DIV0_EVM_MEMORY_H
