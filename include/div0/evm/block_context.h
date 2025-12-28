#ifndef DIV0_EVM_BLOCK_CONTEXT_H
#define DIV0_EVM_BLOCK_CONTEXT_H

#include "div0/types/address.h"
#include "div0/types/hash.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stdint.h>

/// Callback for BLOCKHASH opcode.
/// @param block_number The block number to look up
/// @param user_data User-provided context
/// @param out Output hash (only written if return is true)
/// @return true if hash was found, false otherwise
typedef bool (*get_block_hash_fn_t)(uint64_t block_number, void *user_data, hash_t *out);

/// Block-level execution context.
/// Shared across all transactions in a block. Set once per block.
typedef struct {
  uint64_t number;                    // NUMBER opcode (0x43)
  uint64_t timestamp;                 // TIMESTAMP opcode (0x42)
  uint64_t gas_limit;                 // GASLIMIT opcode (0x45)
  uint64_t chain_id;                  // CHAINID opcode (0x46)
  uint256_t base_fee;                 // BASEFEE opcode (0x48, EIP-1559)
  uint256_t blob_base_fee;            // BLOBBASEFEE opcode (0x4A, EIP-4844)
  uint256_t prev_randao;              // PREVRANDAO opcode (0x44, post-merge)
  address_t coinbase;                 // COINBASE opcode (0x41)
  get_block_hash_fn_t get_block_hash; // Lazy callback for BLOCKHASH (0x40)
  void *block_hash_user_data;
} block_context_t;

/// Initializes a block context with default values.
static inline void block_context_init(block_context_t *ctx) {
  ctx->number = 0;
  ctx->timestamp = 0;
  ctx->gas_limit = 0;
  ctx->chain_id = 1; // Default to mainnet
  ctx->base_fee = uint256_zero();
  ctx->blob_base_fee = uint256_zero();
  ctx->prev_randao = uint256_zero();
  ctx->coinbase = address_zero();
  ctx->get_block_hash = nullptr;
  ctx->block_hash_user_data = nullptr;
}

#endif // DIV0_EVM_BLOCK_CONTEXT_H
