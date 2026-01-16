#include "div0/types/withdrawal.h"

#include "div0/rlp/encode.h"
#include "div0/trie/mpt.h"
#include "div0/trie/node.h"
#include "div0/types/bytes.h"

bytes_t rlp_encode_withdrawal(div0_arena_t *const arena, const withdrawal_t *const w) {
  // First encode each field to get their sizes
  const bytes_t idx = rlp_encode_u64(arena, w->index);
  const bytes_t val = rlp_encode_u64(arena, w->validator_index);
  const bytes_t addr = rlp_encode_address(arena, &w->address);
  const bytes_t amt = rlp_encode_u64(arena, w->amount);

  // Calculate total payload size
  const size_t payload_size = idx.size + val.size + addr.size + amt.size;

  // RLP list header: 1 byte for payload < 56, otherwise 1 + length bytes
  const size_t header_size = payload_size < 56 ? 1 : 1 + ((payload_size < 256) ? 1 : 2);
  const size_t total_size = header_size + payload_size;

  // Allocate output buffer with exact size needed
  bytes_t out;
  bytes_init_arena(&out, arena);
  (void)bytes_reserve(&out, total_size);

  // Write list header
  if (payload_size < 56) {
    (void)bytes_append_byte(&out, (uint8_t)(0xc0 + payload_size));
  } else if (payload_size < 256) {
    (void)bytes_append_byte(&out, 0xf8);
    (void)bytes_append_byte(&out, (uint8_t)payload_size);
  } else {
    (void)bytes_append_byte(&out, 0xf9);
    (void)bytes_append_byte(&out, (uint8_t)(payload_size >> 8));
    (void)bytes_append_byte(&out, (uint8_t)(payload_size & 0xff));
  }

  // Append encoded fields
  (void)bytes_append(&out, idx.data, idx.size);
  (void)bytes_append(&out, val.data, val.size);
  (void)bytes_append(&out, addr.data, addr.size);
  (void)bytes_append(&out, amt.data, amt.size);

  return out;
}

hash_t compute_withdrawals_root(const withdrawal_t *const withdrawals, const size_t count,
                                div0_arena_t *const arena) {
  if (count == 0) {
    return MPT_EMPTY_ROOT;
  }

  // Create a temporary trie for computing the root
  mpt_t trie;
  mpt_init(&trie, mpt_memory_backend_create(arena), arena);

  for (size_t i = 0; i < count; i++) {
    // Key is RLP-encoded index in the list (not withdrawal.index)
    const bytes_t key = rlp_encode_u64(arena, i);
    // Value is RLP-encoded withdrawal
    const bytes_t value = rlp_encode_withdrawal(arena, &withdrawals[i]);
    (void)mpt_insert(&trie, key.data, key.size, value.data, value.size);
  }

  return mpt_root_hash(&trie);
}
