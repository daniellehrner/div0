#include "div0/crypto/secp256k1.h"

#include <secp256k1.h>
#include <secp256k1_recovery.h>

#include <algorithm>
#include <cstring>
#include <utility>

#include "div0/crypto/keccak256.h"
#include "div0/utils/bytes.h"

namespace div0::crypto {

Secp256k1Context::Secp256k1Context() : ctx_(secp256k1_context_create(SECP256K1_CONTEXT_NONE)) {}

Secp256k1Context::~Secp256k1Context() {
  if (ctx_ != nullptr) {
    secp256k1_context_destroy(static_cast<secp256k1_context*>(ctx_));
  }
}

Secp256k1Context::Secp256k1Context(Secp256k1Context&& other) noexcept
    : ctx_(std::exchange(other.ctx_, nullptr)) {}

Secp256k1Context& Secp256k1Context::operator=(Secp256k1Context&& other) noexcept {
  if (this != &other) {
    if (ctx_ != nullptr) {
      secp256k1_context_destroy(static_cast<secp256k1_context*>(ctx_));
    }
    ctx_ = std::exchange(other.ctx_, nullptr);
  }
  return *this;
}

std::optional<std::array<uint8_t, 64>> Secp256k1Context::recover_public_key(
    std::span<const uint8_t, 32> message_hash, int recovery_id,
    std::span<const uint8_t, 64> signature) const {
  if (recovery_id < 0 || recovery_id > 3) {
    return std::nullopt;
  }

  auto* ctx = static_cast<secp256k1_context*>(ctx_);

  secp256k1_ecdsa_recoverable_signature sig;
  if (secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &sig, signature.data(),
                                                          recovery_id) == 0) {
    return std::nullopt;
  }

  secp256k1_pubkey pubkey;
  if (secp256k1_ecdsa_recover(ctx, &pubkey, &sig, message_hash.data()) == 0) {
    return std::nullopt;
  }

  std::array<uint8_t, 65> serialized{};
  size_t output_len = 65;
  secp256k1_ec_pubkey_serialize(ctx, serialized.data(), &output_len, &pubkey,
                                SECP256K1_EC_UNCOMPRESSED);

  // Skip the 0x04 prefix byte, return only the 64-byte public key
  std::array<uint8_t, 64> result{};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  std::copy_n(serialized.begin() + 1, 64, result.begin());
  return result;
}

std::optional<types::Address> Secp256k1Context::ecrecover(const types::Uint256& message_hash,
                                                          uint64_t v, const types::Uint256& r,
                                                          const types::Uint256& s,
                                                          std::optional<uint64_t> chain_id) const {
  // Decode recovery ID from v
  int recovery_id = 0;
  if (v == 27 || v == 28) {
    // Pre-EIP-155
    recovery_id = static_cast<int>(v - 27);
  } else if (chain_id.has_value()) {
    // EIP-155: v = chain_id * 2 + 35 + recovery_id
    const uint64_t expected_base = (chain_id.value() * 2) + 35;
    if (v != expected_base && v != expected_base + 1) {
      return std::nullopt;
    }
    recovery_id = static_cast<int>(v - expected_base);
  } else {
    // Try to decode as EIP-155 without known chain_id
    // v >= 35 means EIP-155, recovery_id = (v - 35) % 2
    if (v >= 35) {
      recovery_id = static_cast<int>((v - 35) % 2);
    } else {
      return std::nullopt;
    }
  }

  // Convert Uint256 values to big-endian bytes using swap_endian_256
  // Uint256 stores limbs in little-endian order, swap_endian_256 converts to big-endian bytes
  std::array<uint64_t, 4> r_limbs = {r.limb(0), r.limb(1), r.limb(2), r.limb(3)};
  std::array<uint64_t, 4> s_limbs = {s.limb(0), s.limb(1), s.limb(2), s.limb(3)};
  std::array<uint64_t, 4> hash_limbs = {message_hash.limb(0), message_hash.limb(1),
                                        message_hash.limb(2), message_hash.limb(3)};

  auto r_be = utils::swap_endian_256(r_limbs);
  auto s_be = utils::swap_endian_256(s_limbs);
  auto hash_be = utils::swap_endian_256(hash_limbs);

  // Build signature bytes (r || s)
  std::array<uint8_t, 64> sig_bytes{};
  std::memcpy(sig_bytes.data(), r_be.data(), 32);
  std::memcpy(&sig_bytes[32], s_be.data(),
              32);  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)

  // Build message hash bytes
  std::array<uint8_t, 32> hash_bytes{};
  std::memcpy(hash_bytes.data(), hash_be.data(), 32);

  // Recover public key
  auto pubkey_opt = recover_public_key(hash_bytes, recovery_id, sig_bytes);
  if (!pubkey_opt) {
    return std::nullopt;
  }

  // Address = last 20 bytes of keccak256(public_key)
  auto pubkey_hash = keccak256(*pubkey_opt);

  // Convert pubkey_hash to big-endian bytes
  std::array<uint64_t, 4> pubkey_hash_limbs = {pubkey_hash.limb(0), pubkey_hash.limb(1),
                                               pubkey_hash.limb(2), pubkey_hash.limb(3)};
  auto pubkey_hash_be = utils::swap_endian_256(pubkey_hash_limbs);

  // Extract last 20 bytes from the 32-byte hash
  std::array<uint8_t, 32> pubkey_hash_bytes{};
  std::memcpy(pubkey_hash_bytes.data(), pubkey_hash_be.data(), 32);

  std::array<uint8_t, 20> addr_bytes{};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  std::copy_n(pubkey_hash_bytes.begin() + 12, 20, addr_bytes.begin());

  return types::Address(std::span<const uint8_t, 20>(addr_bytes));
}

}  // namespace div0::crypto
