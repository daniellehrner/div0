#include "div0/crypto/keccak256.h"

#include "KeccakSponge.h"

#include <assert.h>
#include <string.h>

// Keccak-256 parameters (in bits)
// rate + capacity = 1600 (the Keccak-f[1600] permutation width)
static constexpr unsigned KECCAK256_RATE = 1088;    // 136 bytes
static constexpr unsigned KECCAK256_CAPACITY = 512; // 64 bytes

// Ensure our storage is large enough for any XKCP target
static_assert(sizeof(KeccakWidth1600_SpongeInstance) <= KECCAK256_STATE_SIZE,
              "Increase KECCAK256_STATE_SIZE to fit KeccakWidth1600_SpongeInstance");

// Helper to access typed sponge state from raw storage.
// Safety: hasher->state is aligned to 64 bytes (alignas(64) in header),
// which exceeds any alignment requirement of KeccakWidth1600_SpongeInstance.
static inline KeccakWidth1600_SpongeInstance *sponge(keccak256_hasher_t *hasher) {
  // NOLINTNEXTLINE(bugprone-casting-through-void)
  return (KeccakWidth1600_SpongeInstance *)(void *)hasher->state;
}

void keccak256_init(keccak256_hasher_t *hasher) {
  int result = KeccakWidth1600_SpongeInitialize(sponge(hasher), KECCAK256_RATE, KECCAK256_CAPACITY);
  assert(result == 0 && "Keccak sponge initialization failed");
  (void)result; // suppress unused warning in release builds
}

void keccak256_update(keccak256_hasher_t *hasher, const uint8_t *data, size_t len) {
  if (len == 0) {
    return;
  }

  int result = KeccakWidth1600_SpongeAbsorb(sponge(hasher), data, len);
  assert(result == 0 && "Keccak absorb failed");
  (void)result;
}

hash_t keccak256_finalize(keccak256_hasher_t *hasher) {
  hash_t output = hash_zero();

  int result = KeccakWidth1600_SpongeSqueeze(sponge(hasher), output.bytes, HASH_SIZE);
  assert(result == 0 && "Keccak squeeze failed");
  (void)result;

  // Reset for reuse
  keccak256_reset(hasher);

  return output;
}

void keccak256_reset(keccak256_hasher_t *hasher) {
  int result = KeccakWidth1600_SpongeInitialize(sponge(hasher), KECCAK256_RATE, KECCAK256_CAPACITY);
  assert(result == 0 && "Keccak reset failed");
  (void)result;
}

void keccak256_destroy(keccak256_hasher_t *hasher) {
  // Securely zero the state to prevent hash state leakage.
  // Use volatile to prevent compiler from optimizing away the zeroing.
  volatile uint8_t *p = hasher->state;
  for (size_t i = 0; i < KECCAK256_STATE_SIZE; i++) {
    p[i] = 0;
  }
}

hash_t keccak256(const uint8_t *data, size_t len) {
  keccak256_hasher_t hasher;
  keccak256_init(&hasher);
  keccak256_update(&hasher, data, len);
  hash_t result = keccak256_finalize(&hasher);
  keccak256_destroy(&hasher);
  return result;
}