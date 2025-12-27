#include "div0/types/address.h"

#include "div0/types/uint256.h"

#include <stdint.h>

// Address is 20 bytes (160 bits), stored big-endian.
// uint256 uses 4 little-endian 64-bit limbs.
//
// Address bytes layout (big-endian):
//   bytes[0..3]   = upper 32 bits (from limbs[2] lower 32 bits)
//   bytes[4..11]  = middle 64 bits (from limbs[1])
//   bytes[12..19] = lower 64 bits (from limbs[0])

// NOLINTBEGIN(readability-magic-numbers)

address_t address_from_uint256(const uint256_t *value) {
  address_t result;

  // Extract limbs[2] lower 32 bits -> bytes[0..3] (big-endian)
  uint32_t upper = (uint32_t)value->limbs[2];
  result.bytes[0] = (uint8_t)(upper >> 24);
  result.bytes[1] = (uint8_t)(upper >> 16);
  result.bytes[2] = (uint8_t)(upper >> 8);
  result.bytes[3] = (uint8_t)upper;

  // Extract limbs[1] -> bytes[4..11] (big-endian)
  uint64_t mid = value->limbs[1];
  result.bytes[4] = (uint8_t)(mid >> 56);
  result.bytes[5] = (uint8_t)(mid >> 48);
  result.bytes[6] = (uint8_t)(mid >> 40);
  result.bytes[7] = (uint8_t)(mid >> 32);
  result.bytes[8] = (uint8_t)(mid >> 24);
  result.bytes[9] = (uint8_t)(mid >> 16);
  result.bytes[10] = (uint8_t)(mid >> 8);
  result.bytes[11] = (uint8_t)mid;

  // Extract limbs[0] -> bytes[12..19] (big-endian)
  uint64_t low = value->limbs[0];
  result.bytes[12] = (uint8_t)(low >> 56);
  result.bytes[13] = (uint8_t)(low >> 48);
  result.bytes[14] = (uint8_t)(low >> 40);
  result.bytes[15] = (uint8_t)(low >> 32);
  result.bytes[16] = (uint8_t)(low >> 24);
  result.bytes[17] = (uint8_t)(low >> 16);
  result.bytes[18] = (uint8_t)(low >> 8);
  result.bytes[19] = (uint8_t)low;

  return result;
}

uint256_t address_to_uint256(const address_t *addr) {
  uint256_t result = uint256_zero();

  // bytes[0..3] -> limbs[2] lower 32 bits
  result.limbs[2] = ((uint64_t)addr->bytes[0] << 24) | ((uint64_t)addr->bytes[1] << 16) |
                    ((uint64_t)addr->bytes[2] << 8) | (uint64_t)addr->bytes[3];

  // bytes[4..11] -> limbs[1]
  result.limbs[1] = ((uint64_t)addr->bytes[4] << 56) | ((uint64_t)addr->bytes[5] << 48) |
                    ((uint64_t)addr->bytes[6] << 40) | ((uint64_t)addr->bytes[7] << 32) |
                    ((uint64_t)addr->bytes[8] << 24) | ((uint64_t)addr->bytes[9] << 16) |
                    ((uint64_t)addr->bytes[10] << 8) | (uint64_t)addr->bytes[11];

  // bytes[12..19] -> limbs[0]
  result.limbs[0] = ((uint64_t)addr->bytes[12] << 56) | ((uint64_t)addr->bytes[13] << 48) |
                    ((uint64_t)addr->bytes[14] << 40) | ((uint64_t)addr->bytes[15] << 32) |
                    ((uint64_t)addr->bytes[16] << 24) | ((uint64_t)addr->bytes[17] << 16) |
                    ((uint64_t)addr->bytes[18] << 8) | (uint64_t)addr->bytes[19];

  // limbs[3] is zero (upper 64 bits)

  return result;
}

// NOLINTEND(readability-magic-numbers)