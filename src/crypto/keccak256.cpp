#include "div0/crypto/keccak256.h"

#include <array>
#include <cassert>
#include <cstring>

#include "div0/utils/bytes.h"

// XKCP C headers - extern "C" prevents C++ name mangling
extern "C" {
#include "KeccakSponge.h"
}

namespace div0::crypto {

namespace {

// Keccak-256 parameters (in bits)
// rate + capacity = 1600 (the Keccak-f[1600] permutation width)
constexpr unsigned KECCAK256_RATE = 1088;     // 136 bytes
constexpr unsigned KECCAK256_CAPACITY = 512;  // 64 bytes

// Ensure our storage is large enough for any XKCP target
static_assert(sizeof(KeccakWidth1600_SpongeInstance) <= Keccak256Hasher::STATE_STORAGE_SIZE,
              "Increase STATE_STORAGE_SIZE to fit KeccakWidth1600_SpongeInstance");

// Helper to access typed sponge state from raw storage
// Uses static_cast through void* to avoid reinterpret_cast (cppcoreguidelines)
inline auto* sponge(uint8_t* storage) noexcept {
  // NOLINTNEXTLINE(misc-const-correctness) - need non-const for sponge modification
  void* ptr = storage;
  return static_cast<KeccakWidth1600_SpongeInstance*>(ptr);
}

/**
 * @brief Convert 32 big-endian bytes to Uint256 (little-endian limbs).
 *
 * Keccak outputs bytes in big-endian order (byte[0] is most significant).
 * Uint256 stores limbs in little-endian order (data_[0] is least significant).
 */
[[nodiscard]] types::Uint256 bytes_to_uint256(const uint8_t* bytes) noexcept {
  // Read 32 bytes as 4 uint64_t (in big-endian byte order)
  std::array<uint64_t, 4> raw{};
  std::memcpy(raw.data(), bytes, 32);

  // Swap byte order and reverse limb order
  const auto swapped = utils::swap_endian_256(raw);

  return types::Uint256(swapped);
}

}  // namespace

// ============================================================================
// Keccak256Hasher Implementation
// ============================================================================

Keccak256Hasher::Keccak256Hasher() noexcept {
  [[maybe_unused]] const int result = KeccakWidth1600_SpongeInitialize(
      sponge(state_storage_.data()), KECCAK256_RATE, KECCAK256_CAPACITY);
  assert(result == 0 && "Keccak sponge initialization failed");
}

Keccak256Hasher::~Keccak256Hasher() noexcept {
  // Securely zero the state to prevent hash state leakage
  // Use volatile to prevent compiler from optimizing away the memset
  for (auto& byte : state_storage_) {
    volatile auto* v = &byte;
    *v = 0;
  }
}

void Keccak256Hasher::update(std::span<const uint8_t> data) noexcept {
  if (data.empty()) {
    return;
  }

  [[maybe_unused]] const int result =
      KeccakWidth1600_SpongeAbsorb(sponge(state_storage_.data()), data.data(), data.size());
  assert(result == 0 && "Keccak absorb failed");
}

types::Uint256 Keccak256Hasher::finalize() noexcept {
  constexpr size_t HASH_SIZE = 32;  // 256 bits
  std::array<uint8_t, HASH_SIZE> output{};

  [[maybe_unused]] const int result =
      KeccakWidth1600_SpongeSqueeze(sponge(state_storage_.data()), output.data(), HASH_SIZE);
  assert(result == 0 && "Keccak squeeze failed");

  // Reset for reuse
  reset();

  return bytes_to_uint256(output.data());
}

void Keccak256Hasher::reset() noexcept {
  [[maybe_unused]] const int result = KeccakWidth1600_SpongeInitialize(
      sponge(state_storage_.data()), KECCAK256_RATE, KECCAK256_CAPACITY);
  assert(result == 0 && "Keccak reset failed");
}

// ============================================================================
// One-Shot API
// ============================================================================

types::Uint256 keccak256(std::span<const uint8_t> data) noexcept {
  Keccak256Hasher hasher;
  hasher.update(data);
  return hasher.finalize();
}

}  // namespace div0::crypto
