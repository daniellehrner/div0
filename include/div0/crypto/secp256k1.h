#ifndef DIV0_CRYPTO_SECP256K1_H
#define DIV0_CRYPTO_SECP256K1_H

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::crypto {

/**
 * @brief RAII wrapper for secp256k1 context.
 *
 * Each instance owns a secp256k1 context. Create one per thread for
 * parallel signature recovery without contention.
 *
 * THREAD SAFETY:
 * - A single instance must not be used concurrently from multiple threads
 * - Different instances can be used concurrently from different threads
 *
 * @code
 *   Secp256k1Context crypto;
 *   auto sender = crypto.ecrecover(hash, v, r, s);
 * @endcode
 */
class Secp256k1Context {
 public:
  Secp256k1Context();
  ~Secp256k1Context();

  Secp256k1Context(const Secp256k1Context&) = delete;
  Secp256k1Context& operator=(const Secp256k1Context&) = delete;
  Secp256k1Context(Secp256k1Context&& other) noexcept;
  Secp256k1Context& operator=(Secp256k1Context&& other) noexcept;

  /**
   * @brief Recover sender address from ECDSA signature.
   *
   * Used for transaction sender recovery and the ECRECOVER precompile (0x01).
   * Supports both pre-EIP-155 and EIP-155 signature formats.
   *
   * Pre-EIP-155: v = 27 or 28
   * EIP-155:     v = chain_id * 2 + 35 + recovery_id
   *
   * @param message_hash Keccak256 hash of the signed message (32 bytes)
   * @param v Recovery ID (27, 28, or EIP-155 encoded)
   * @param r Signature R component (32 bytes, big-endian)
   * @param s Signature S component (32 bytes, big-endian)
   * @param chain_id Optional chain ID for EIP-155 signature validation
   * @return Recovered address, or nullopt if signature is invalid
   */
  [[nodiscard]] std::optional<types::Address> ecrecover(
      const types::Uint256& message_hash, uint64_t v, const types::Uint256& r,
      const types::Uint256& s, std::optional<uint64_t> chain_id = std::nullopt) const;

  /**
   * @brief Recover 64-byte uncompressed public key from ECDSA signature.
   *
   * Low-level function that returns the raw public key bytes (without 0x04 prefix).
   * The address is derived by taking keccak256(pubkey)[12:32].
   *
   * @param message_hash Keccak256 hash of the signed message (32 bytes)
   * @param recovery_id Recovery ID (0 or 1)
   * @param signature 64-byte compact signature (r || s)
   * @return 64-byte uncompressed public key, or nullopt if recovery fails
   */
  [[nodiscard]] std::optional<std::array<uint8_t, 64>> recover_public_key(
      std::span<const uint8_t, 32> message_hash, int recovery_id,
      std::span<const uint8_t, 64> signature) const;

 private:
  void* ctx_;  // Opaque pointer to secp256k1_context
};

}  // namespace div0::crypto

#endif  // DIV0_CRYPTO_SECP256K1_H
