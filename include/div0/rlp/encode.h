#ifndef DIV0_RLP_ENCODE_H
#define DIV0_RLP_ENCODE_H

#include "div0/mem/arena.h"
#include "div0/rlp/helpers.h"
#include "div0/types/address.h"
#include "div0/types/bytes.h"
#include "div0/types/uint256.h"

#include <stddef.h>
#include <stdint.h>

/// Encode a byte array as RLP.
/// @param arena Arena for allocation
/// @param data Data to encode (can be nullptr if len is 0)
/// @param len Length of data
/// @return Arena-backed bytes_t containing encoded data
[[nodiscard]] bytes_t rlp_encode_bytes(div0_arena_t *arena, const uint8_t *data, size_t len);

/// Encode a uint64 value as RLP.
/// @param arena Arena for allocation
/// @param value Value to encode
/// @return Arena-backed bytes_t containing encoded data
[[nodiscard]] bytes_t rlp_encode_u64(div0_arena_t *arena, uint64_t value);

/// Encode a uint256 value as RLP.
/// @param arena Arena for allocation
/// @param value Pointer to value
/// @return Arena-backed bytes_t containing encoded data
[[nodiscard]] bytes_t rlp_encode_uint256(div0_arena_t *arena, const uint256_t *value);

/// Encode an address (20 bytes) as RLP.
/// @param arena Arena for allocation
/// @param addr Pointer to address
/// @return Arena-backed bytes_t containing encoded data
[[nodiscard]] bytes_t rlp_encode_address(div0_arena_t *arena, const address_t *addr);

/// List builder for RLP encoding.
/// Reserves space for header, allows appending items, then backfills header.
typedef struct {
  bytes_t *output;   // The bytes_t being built
  size_t header_pos; // Position where header space was reserved
} rlp_list_builder_t;

/// Start building an RLP list.
/// Reserves 9 bytes for the header (maximum possible header size).
/// @param builder Builder to initialize
/// @param output bytes_t to append to (must be initialized)
void rlp_list_start(rlp_list_builder_t *builder, bytes_t *output);

/// Finish building an RLP list.
/// Calculates payload size, writes actual header, compacts if needed.
/// @param builder Builder to finalize
void rlp_list_end(rlp_list_builder_t *builder);

/// Append an already-encoded item to a list being built.
/// @param output The bytes_t being built
/// @param item Encoded item to append
static inline void rlp_list_append(bytes_t *output, const bytes_t *item) {
  bytes_append(output, item->data, item->size);
}

/// Append raw bytes to a list being built.
/// @param output The bytes_t being built
/// @param data Data to append
/// @param len Length of data
static inline void rlp_list_append_raw(bytes_t *output, const uint8_t *data, size_t len) {
  bytes_append(output, data, len);
}

#endif // DIV0_RLP_ENCODE_H
