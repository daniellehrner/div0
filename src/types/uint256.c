#include "div0/types/uint256.h"

#include <stdint.h>
#include <string.h>

// Constants for uint256 byte operations
constexpr size_t UINT256_BYTES = 32;
constexpr int UINT256_LIMBS = 4;
constexpr int BYTES_PER_LIMB = 8;
constexpr uint8_t BYTE_MASK = 0xFF;
constexpr int BITS_PER_BYTE = 8;

uint256_t uint256_from_bytes_be(const uint8_t *data, size_t len) {
  uint256_t result = uint256_zero();

  if (len == 0 || data == nullptr) {
    return result;
  }

  // Clamp length to 32 bytes
  if (len > UINT256_BYTES) {
    data += len - UINT256_BYTES;
    len = UINT256_BYTES;
  }

  // Convert big-endian bytes to little-endian limbs
  // Big-endian: data[0] is MSB, data[len-1] is LSB
  // Little-endian limbs: limbs[0] is least significant
  uint8_t padded[UINT256_BYTES] = {0};
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(padded + (UINT256_BYTES - len), data, len);

  // Now padded[0..7] goes to limbs[3] (MSB)
  // padded[24..31] goes to limbs[0] (LSB)
  for (int i = 0; i < UINT256_LIMBS; i++) {
    uint64_t limb = 0;
    size_t offset = (size_t)((UINT256_LIMBS - 1) - i) * (size_t)BYTES_PER_LIMB;
    const uint8_t *p = padded + offset;
    for (int j = 0; j < BYTES_PER_LIMB; j++) {
      limb = (limb << BITS_PER_BYTE) | p[j];
    }
    result.limbs[i] = limb;
  }

  return result;
}

void uint256_to_bytes_be(uint256_t value, uint8_t *out) {
  // Convert little-endian limbs to big-endian bytes
  // limbs[3] (MSB) goes to out[0..7]
  // limbs[0] (LSB) goes to out[24..31]
  for (int i = 0; i < UINT256_LIMBS; i++) {
    uint64_t limb = value.limbs[(UINT256_LIMBS - 1) - i];
    uint8_t *p = out + ((size_t)i * BYTES_PER_LIMB);
    for (int j = BYTES_PER_LIMB - 1; j >= 0; j--) {
      p[j] = (uint8_t)(limb & BYTE_MASK);
      limb >>= BITS_PER_BYTE;
    }
  }
}
