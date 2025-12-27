#include "div0/crypto/secp256k1.h"

#include "div0/crypto/keccak256.h"

#include <secp256k1.h>
#include <secp256k1_recovery.h>
#include <stdlib.h>
#include <string.h>

// Opaque context structure
struct secp256k1_ctx {
  secp256k1_context *ctx;
};

secp256k1_ctx_t *secp256k1_ctx_create(void) {
  secp256k1_ctx_t *wrapper = malloc(sizeof(secp256k1_ctx_t));
  if (wrapper == nullptr) {
    return nullptr;
  }

  wrapper->ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
  if (wrapper->ctx == nullptr) {
    free(wrapper);
    return nullptr;
  }

  return wrapper;
}

void secp256k1_ctx_destroy(secp256k1_ctx_t *ctx) {
  if (ctx != nullptr) {
    if (ctx->ctx != nullptr) {
      secp256k1_context_destroy(ctx->ctx);
    }
    free(ctx);
  }
}

pubkey_result_t secp256k1_recover_pubkey(const secp256k1_ctx_t *ctx, const uint8_t message_hash[32],
                                         int recovery_id, const uint8_t signature[64]) {
  pubkey_result_t result = {.success = false, .pubkey = {0}};

  if (recovery_id < 0 || recovery_id > 3) {
    return result;
  }

  secp256k1_ecdsa_recoverable_signature sig;
  if (secp256k1_ecdsa_recoverable_signature_parse_compact(ctx->ctx, &sig, signature, recovery_id) ==
      0) {
    return result;
  }

  secp256k1_pubkey pubkey;
  if (secp256k1_ecdsa_recover(ctx->ctx, &pubkey, &sig, message_hash) == 0) {
    return result;
  }

  uint8_t serialized[65];
  size_t output_len = 65;
  secp256k1_ec_pubkey_serialize(ctx->ctx, serialized, &output_len, &pubkey,
                                SECP256K1_EC_UNCOMPRESSED);

  // Skip the 0x04 prefix byte, copy only the 64-byte public key
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(result.pubkey, serialized + 1, 64);
  result.success = true;
  return result;
}

ecrecover_result_t secp256k1_ecrecover(const secp256k1_ctx_t *ctx, const uint256_t *message_hash,
                                       uint64_t v, const uint256_t *r, const uint256_t *s,
                                       uint64_t chain_id) {
  ecrecover_result_t result = {.success = false, .address = address_zero()};

  // Decode recovery ID from v
  int recovery_id = 0;
  if (v == 27 || v == 28) {
    // Pre-EIP-155
    recovery_id = (int)(v - 27);
  } else if (chain_id > 0) {
    // EIP-155: v = chain_id * 2 + 35 + recovery_id
    uint64_t expected_base = (chain_id * 2) + 35;
    if (v != expected_base && v != expected_base + 1) {
      return result;
    }
    recovery_id = (int)(v - expected_base);
  } else {
    // Try to decode as EIP-155 without known chain_id
    // v >= 35 means EIP-155, recovery_id = (v - 35) % 2
    if (v >= 35) {
      recovery_id = (int)((v - 35) % 2);
    } else {
      return result;
    }
  }

  // Convert uint256 values to big-endian bytes
  uint8_t hash_be[32];
  uint8_t r_be[32];
  uint8_t s_be[32];
  uint256_to_bytes_be(*message_hash, hash_be);
  uint256_to_bytes_be(*r, r_be);
  uint256_to_bytes_be(*s, s_be);

  // Build signature bytes (r || s)
  uint8_t sig_bytes[64];
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(sig_bytes, r_be, 32);
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(sig_bytes + 32, s_be, 32);

  // Recover public key
  pubkey_result_t pk = secp256k1_recover_pubkey(ctx, hash_be, recovery_id, sig_bytes);
  if (!pk.success) {
    return result;
  }

  // Address = last 20 bytes of keccak256(public_key)
  hash_t pubkey_hash = keccak256(pk.pubkey, 64);

  // Extract last 20 bytes from the 32-byte hash (bytes 12-31)
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(result.address.bytes, pubkey_hash.bytes + 12, ADDRESS_SIZE);
  result.success = true;
  return result;
}