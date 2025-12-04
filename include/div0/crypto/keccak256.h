#ifndef DIV0_CRYPTO_KECCAK256_H
#define DIV0_CRYPTO_KECCAK256_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "div0/types/hash.h"

namespace div0::crypto {

/**
 * @brief RAII wrapper for incremental Keccak-256 hashing.
 *
 * Wraps XKCP's sponge construction with automatic initialization and cleanup.
 * The sponge state is stored inline (no heap allocation) for performance.
 *
 * USAGE (incremental):
 * @code
 *   Keccak256Hasher hasher;
 *   hasher.update(chunk1);
 *   hasher.update(chunk2);
 *   types::Hash result = hasher.finalize();
 * @endcode
 *
 * THREAD SAFETY:
 * - NOT thread-safe: each thread should have its own Hasher instance
 * - The hasher is reusable after finalize() - automatically resets
 *
 * @note Copy/move deleted to prevent sponge state misuse
 */
class Keccak256Hasher {
 public:
  /**
   * @brief Initialize sponge state for Keccak-256.
   *
   * Keccak-256 parameters: rate=1088 bits, capacity=512 bits
   */
  Keccak256Hasher() noexcept;

  /**
   * @brief Clean up sponge state.
   *
   * Securely zeros the state to prevent hash state leakage.
   */
  ~Keccak256Hasher() noexcept;

  // Non-copyable, non-movable (stateful resource)
  Keccak256Hasher(const Keccak256Hasher&) = delete;
  Keccak256Hasher& operator=(const Keccak256Hasher&) = delete;
  Keccak256Hasher(Keccak256Hasher&&) = delete;
  Keccak256Hasher& operator=(Keccak256Hasher&&) = delete;

  /**
   * @brief Absorb data into the sponge.
   *
   * Can be called multiple times before finalize().
   *
   * @param data Data to hash
   */
  void update(std::span<const uint8_t> data) noexcept;

  /**
   * @brief Finalize and squeeze out the 256-bit hash.
   *
   * Automatically resets the hasher for reuse.
   *
   * @return 256-bit hash
   */
  [[nodiscard]] types::Hash finalize() noexcept;

  /**
   * @brief Reset to initial state (for reuse without reconstruction).
   */
  void reset() noexcept;

  // Size of inline storage for sponge state (public for static_assert in impl)
  static constexpr size_t STATE_STORAGE_SIZE = 256;

 private:
  // Aligned storage for XKCP sponge instance (avoids heap allocation).
  // Size is conservative to accommodate any XKCP target (AVX2, NEON, generic).
  // The actual KeccakWidth1600_SpongeInstance is ~220 bytes.
  alignas(64) std::array<uint8_t, STATE_STORAGE_SIZE> state_storage_{};
};

// ============================================================================
// One-Shot API (convenience functions)
// ============================================================================

/**
 * @brief Compute Keccak-256 hash in a single call.
 *
 * Optimal for small inputs where incremental hashing overhead isn't needed.
 *
 * @param data Input data to hash
 * @return 256-bit hash
 */
[[nodiscard]] types::Hash keccak256(std::span<const uint8_t> data) noexcept;

/**
 * @brief Compute Keccak-256 hash of a fixed-size array.
 *
 * @tparam N Size of input array
 * @param data Input data to hash
 * @return 256-bit hash
 */
template <size_t N>
[[nodiscard]] types::Hash keccak256(const std::array<uint8_t, N>& data) noexcept {
  return keccak256(std::span<const uint8_t>(data));
}

}  // namespace div0::crypto

#endif  // DIV0_CRYPTO_KECCAK256_H
