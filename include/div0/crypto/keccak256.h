#ifndef DIV0_CRYPTO_KECCAK256_H
#define DIV0_CRYPTO_KECCAK256_H

#include "div0/types/hash.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

/// Size of inline storage for sponge state.
/// Accommodates any XKCP target (AVX2, NEON, generic).
/// Actual KeccakWidth1600_SpongeInstance is ~220 bytes.
static constexpr size_t KECCAK256_STATE_SIZE = 256;

/// Incremental Keccak-256 hasher.
///
/// Stores sponge state inline (no heap allocation).
/// Each instance is NOT thread-safe - use one per thread.
///
/// Usage (incremental):
/// @code
///   keccak256_hasher_t hasher;
///   keccak256_init(&hasher);
///   keccak256_update(&hasher, data1, len1);
///   keccak256_update(&hasher, data2, len2);
///   hash_t result = keccak256_finalize(&hasher);
///   keccak256_destroy(&hasher);  // zeros state
/// @endcode
typedef struct {
  alignas(64) uint8_t state[KECCAK256_STATE_SIZE];
} keccak256_hasher_t;

/// Initialize hasher for Keccak-256 (rate=1088, capacity=512).
/// @param hasher Hasher to initialize
void keccak256_init(keccak256_hasher_t *hasher);

/// Absorb data into the sponge.
/// @param hasher Hasher state
/// @param data Data to hash (may be nullptr if len is 0)
/// @param len Length of data in bytes
void keccak256_update(keccak256_hasher_t *hasher, const uint8_t *data, size_t len);

/// Finalize and squeeze out 256-bit hash.
/// Automatically resets hasher for reuse.
/// @param hasher Hasher state
/// @return 256-bit hash
hash_t keccak256_finalize(keccak256_hasher_t *hasher);

/// Reset hasher to initial state.
/// @param hasher Hasher to reset
void keccak256_reset(keccak256_hasher_t *hasher);

/// Securely destroy hasher (zeros state).
/// Call this when done with the hasher to prevent state leakage.
/// @param hasher Hasher to destroy
void keccak256_destroy(keccak256_hasher_t *hasher);

// ============================================================================
// One-Shot API
// ============================================================================

/// Compute Keccak-256 hash in a single call.
/// Optimal for small inputs where incremental hashing overhead isn't needed.
/// @param data Input data to hash (may be nullptr if len is 0)
/// @param len Length of data in bytes
/// @return 256-bit hash
hash_t keccak256(const uint8_t *data, size_t len);

#endif // DIV0_CRYPTO_KECCAK256_H