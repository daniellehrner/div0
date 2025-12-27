#ifndef DIV0_CRYPTO_SECP256K1_H
#define DIV0_CRYPTO_SECP256K1_H

#include "div0/types/address.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// Opaque secp256k1 context.
/// Each instance owns a secp256k1_context. Create one per thread for
/// parallel signature recovery without contention.
///
/// Thread safety:
/// - A single instance must not be used concurrently from multiple threads
/// - Different instances can be used concurrently from different threads
typedef struct secp256k1_ctx secp256k1_ctx_t;

/// Create a new secp256k1 context.
/// @return New context, or nullptr on allocation failure
secp256k1_ctx_t *secp256k1_ctx_create(void);

/// Destroy a secp256k1 context.
/// @param ctx Context to destroy (may be nullptr)
void secp256k1_ctx_destroy(secp256k1_ctx_t *ctx);

/// Result of ecrecover operation.
typedef struct {
  bool success;      ///< true if recovery succeeded
  address_t address; ///< recovered address (valid only if success)
} ecrecover_result_t;

/// Recover sender address from ECDSA signature.
///
/// Used for transaction sender recovery and the ECRECOVER precompile (0x01).
/// Supports both pre-EIP-155 and EIP-155 signature formats.
///
/// Pre-EIP-155: v = 27 or 28
/// EIP-155:     v = chain_id * 2 + 35 + recovery_id
///
/// @param ctx secp256k1 context
/// @param message_hash Keccak256 hash of signed message
/// @param v Recovery ID (27, 28, or EIP-155 encoded)
/// @param r Signature R component
/// @param s Signature S component
/// @param chain_id Chain ID for EIP-155 (0 to auto-detect from v)
/// @return Result containing success flag and recovered address
ecrecover_result_t secp256k1_ecrecover(const secp256k1_ctx_t *ctx, const uint256_t *message_hash,
                                       uint64_t v, const uint256_t *r, const uint256_t *s,
                                       uint64_t chain_id);

/// Result of public key recovery.
typedef struct {
  bool success;       ///< true if recovery succeeded
  uint8_t pubkey[64]; ///< 64-byte uncompressed public key (no 0x04 prefix)
} pubkey_result_t;

/// Recover 64-byte uncompressed public key from signature.
///
/// Low-level function; address = keccak256(pubkey)[12:32].
///
/// @param ctx secp256k1 context
/// @param message_hash 32-byte message hash
/// @param recovery_id Recovery ID (0-3)
/// @param signature 64-byte compact signature (r || s)
/// @return Result containing success flag and public key
pubkey_result_t secp256k1_recover_pubkey(const secp256k1_ctx_t *ctx, const uint8_t message_hash[32],
                                         int recovery_id, const uint8_t signature[64]);

#endif // DIV0_CRYPTO_SECP256K1_H