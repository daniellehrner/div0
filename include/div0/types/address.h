#ifndef DIV0_TYPES_ADDRESS_H
#define DIV0_TYPES_ADDRESS_H

#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/// Size of Ethereum address in bytes.
static constexpr size_t ADDRESS_SIZE = 20;

/// 160-bit Ethereum address.
/// Storage is big-endian (EVM convention).
typedef struct {
  uint8_t bytes[ADDRESS_SIZE];
} address_t;

static_assert(sizeof(address_t) == ADDRESS_SIZE, "address_t must be 20 bytes");

/// Returns a zero-initialized address.
static inline address_t address_zero(void) {
  return (address_t){{0}};
}

/// Checks if an address is zero.
static inline bool address_is_zero(const address_t *addr) {
  for (size_t i = 0; i < ADDRESS_SIZE; i++) {
    if (addr->bytes[i] != 0) {
      return false;
    }
  }
  return true;
}

/// Compares two addresses for equality.
static inline bool address_equal(const address_t *a, const address_t *b) {
  return memcmp(a->bytes, b->bytes, ADDRESS_SIZE) == 0;
}

/// Creates an address from a 20-byte array.
/// @param data Source data (must be exactly 20 bytes)
/// @return address containing the data
static inline address_t address_from_bytes(const uint8_t *data) {
  address_t result;
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(result.bytes, data, ADDRESS_SIZE);
  return result;
}

/// Creates an address from a uint256 (extracts lower 160 bits).
/// @param value Source uint256
/// @return address containing the lower 160 bits
address_t address_from_uint256(const uint256_t *value);

/// Converts an address to a uint256.
/// The address occupies the lower 160 bits; upper 96 bits are zero.
/// @param addr Source address
/// @return uint256 representation
uint256_t address_to_uint256(const address_t *addr);

#endif // DIV0_TYPES_ADDRESS_H