#include "div0/ethereum/bloom.h"

#include <cstring>

#include "div0/crypto/keccak256.h"
#include "div0/utils/bytes.h"

namespace div0::ethereum {

Bloom::Bloom(std::span<const uint8_t, SIZE> bytes) noexcept {
  std::memcpy(data_.data(), bytes.data(), SIZE);
}

void Bloom::add(const types::Address& address) noexcept { add(address.span()); }

void Bloom::add(const types::Uint256& topic) noexcept {
  // Convert Uint256 (little-endian limbs) to big-endian bytes for hashing
  const auto be_limbs = utils::swap_endian_256(topic.data_unsafe());
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  add(std::span<const uint8_t, 32>(reinterpret_cast<const uint8_t*>(be_limbs.data()), 32));
}

void Bloom::add(std::span<const uint8_t> data) noexcept {
  const types::Hash hash = crypto::keccak256(data);
  set_bits_from_hash(hash.span());
}

bool Bloom::may_contain(const types::Address& address) const noexcept {
  return may_contain(address.span());
}

bool Bloom::may_contain(const types::Uint256& topic) const noexcept {
  const auto be_limbs = utils::swap_endian_256(topic.data_unsafe());
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* bytes = reinterpret_cast<const uint8_t*>(be_limbs.data());
  return may_contain(std::span<const uint8_t, 32>(bytes, 32));
}

bool Bloom::may_contain(std::span<const uint8_t> data) const noexcept {
  const types::Hash hash = crypto::keccak256(data);
  return has_bits_from_hash(hash.span());
}

Bloom& Bloom::operator|=(const Bloom& other) noexcept {
  // Use 64-bit word operations for efficiency (256 bytes = 32 words)
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto* dst = reinterpret_cast<uint64_t*>(data_.data());
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* src = reinterpret_cast<const uint64_t*>(other.data_.data());
  for (size_t i = 0; i < SIZE / sizeof(uint64_t); ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    dst[i] |= src[i];
  }
  return *this;
}

Bloom Bloom::operator|(const Bloom& other) const noexcept {
  Bloom result = *this;
  result |= other;
  return result;
}

bool Bloom::is_empty() const noexcept {
  // Use 64-bit accumulator for efficiency
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* data = reinterpret_cast<const uint64_t*>(data_.data());
  uint64_t acc = 0;
  for (size_t i = 0; i < SIZE / sizeof(uint64_t); ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    acc |= data[i];
  }
  return acc == 0;
}

void Bloom::clear() noexcept { data_.fill(0); }

void Bloom::set_bits_from_hash(std::span<const uint8_t, 32> hash) noexcept {
  // Ethereum bloom uses 3 bit positions derived from the hash
  // Each position comes from 2 bytes: bytes[0,1], [2,3], [4,5]
  set_bit(bit_index(hash[0], hash[1]));
  set_bit(bit_index(hash[2], hash[3]));
  set_bit(bit_index(hash[4], hash[5]));
}

bool Bloom::has_bits_from_hash(std::span<const uint8_t, 32> hash) const noexcept {
  return has_bit(bit_index(hash[0], hash[1])) && has_bit(bit_index(hash[2], hash[3])) &&
         has_bit(bit_index(hash[4], hash[5]));
}

void Bloom::set_bit(uint16_t index) noexcept {
  // Bloom filter is stored big-endian: bit 0 is in byte 255, bit 2047 is in byte 0
  const auto byte_index = SIZE - 1 - (index / 8);
  const auto bit_mask = static_cast<uint8_t>(1 << (index % 8));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  data_[byte_index] |= bit_mask;
}

bool Bloom::has_bit(uint16_t index) const noexcept {
  const auto byte_index = SIZE - 1 - (index / 8);
  const auto bit_mask = static_cast<uint8_t>(1 << (index % 8));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return (data_[byte_index] & bit_mask) != 0;
}

}  // namespace div0::ethereum
