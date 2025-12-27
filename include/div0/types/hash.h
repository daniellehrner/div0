#ifndef DIV0_TYPES_HASH_H
#define DIV0_TYPES_HASH_H

#include "div0/types/uint256.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/// Size of hash in bytes.
static constexpr size_t HASH_SIZE = 32;

/// 32-byte cryptographic hash (Keccak-256).
/// Storage is big-endian. Aligned for optimal performance.
typedef struct {
  alignas(HASH_SIZE) uint8_t bytes[HASH_SIZE];
} hash_t;

static_assert(sizeof(hash_t) == HASH_SIZE, "hash_t must be 32 bytes");
static_assert(alignof(hash_t) == HASH_SIZE, "hash_t must be 32-byte aligned");

/// Returns a zero-initialized hash.
static inline hash_t hash_zero(void) {
  return (hash_t){{0}};
}

/// Checks if a hash is zero.
static inline bool hash_is_zero(const hash_t *h) {
  // Use 64-bit comparisons for efficiency (aligned access via memcpy)
  uint64_t limbs[4];
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(limbs, h->bytes, HASH_SIZE);
  return (limbs[0] | limbs[1] | limbs[2] | limbs[3]) == 0;
}

/// Compares two hash values for equality.
static inline bool hash_equal(const hash_t *a, const hash_t *b) {
  return memcmp(a->bytes, b->bytes, HASH_SIZE) == 0;
}

/// Creates a hash from a 32-byte array.
/// @param data Source data (must be exactly 32 bytes)
/// @return hash containing the data
static inline hash_t hash_from_bytes(const uint8_t *data) {
  hash_t result;
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(result.bytes, data, HASH_SIZE);
  return result;
}

/// Parse a hash from a hex string.
/// Accepts optional "0x" prefix. Requires exactly 64 hex characters.
/// @param hex Hex string (with or without 0x prefix)
/// @param out Output hash
/// @return true if parsing succeeded, false on invalid input
bool hash_from_hex(const char *hex, hash_t *out);

/// Converts a hash to a uint256.
/// @param h Source hash
/// @return uint256 representation
uint256_t hash_to_uint256(const hash_t *h);

/// Creates a hash from a uint256.
/// @param value Source uint256
/// @return hash representation
hash_t hash_from_uint256(const uint256_t *value);

#endif // DIV0_TYPES_HASH_H