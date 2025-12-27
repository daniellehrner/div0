#ifndef DIV0_RLP_DECODE_H
#define DIV0_RLP_DECODE_H

#include "div0/rlp/helpers.h"
#include "div0/types/address.h"
#include "div0/types/uint256.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// RLP decode error codes.
typedef enum {
  RLP_SUCCESS = 0,          // Decoding succeeded
  RLP_ERR_INPUT_TOO_SHORT,  // Unexpected end of input
  RLP_ERR_LEADING_ZEROS,    // Non-canonical encoding (leading zeros in length)
  RLP_ERR_NON_CANONICAL,    // Could have been encoded more efficiently
  RLP_ERR_INTEGER_OVERFLOW, // Integer too large for target type
  RLP_ERR_LIST_MISMATCH,    // List length doesn't match consumed bytes
  RLP_ERR_INVALID_PREFIX,   // Invalid RLP prefix byte
  RLP_ERR_WRONG_SIZE,       // Data size doesn't match expected size
} rlp_error_t;

/// RLP decoder state.
/// Reads from input buffer, maintains position for sequential decoding.
typedef struct {
  const uint8_t *input; // Input buffer
  size_t size;          // Buffer size
  size_t pos;           // Current read position
} rlp_decoder_t;

/// Result of decoding bytes.
/// Returns a view into input buffer (zero-copy).
typedef struct {
  const uint8_t *data;   // Pointer into input buffer (nullptr on error)
  size_t len;            // Length of data
  size_t bytes_consumed; // Total bytes consumed from input
  rlp_error_t error;     // Error code (RLP_SUCCESS on success)
} rlp_bytes_result_t;

/// Result of decoding uint64.
typedef struct {
  uint64_t value;        // Decoded value (0 on error)
  size_t bytes_consumed; // Total bytes consumed from input
  rlp_error_t error;     // Error code
} rlp_u64_result_t;

/// Result of decoding uint256.
typedef struct {
  uint256_t value;       // Decoded value (zero on error)
  size_t bytes_consumed; // Total bytes consumed from input
  rlp_error_t error;     // Error code
} rlp_uint256_result_t;

/// Result of decoding address.
typedef struct {
  address_t value;       // Decoded address (zero on error)
  size_t bytes_consumed; // Total bytes consumed from input
  rlp_error_t error;     // Error code
} rlp_address_result_t;

/// Result of decoding list header.
typedef struct {
  size_t payload_length; // Length of list payload
  size_t bytes_consumed; // Bytes consumed for header
  rlp_error_t error;     // Error code
} rlp_list_result_t;

/// Initialize decoder with input buffer.
/// @param decoder Decoder to initialize
/// @param input Input buffer
/// @param size Buffer size
static inline void rlp_decoder_init(rlp_decoder_t *decoder, const uint8_t *input, size_t size) {
  decoder->input = input;
  decoder->size = size;
  decoder->pos = 0;
}

/// Decode a byte string, returning a view into the original input (zero-copy).
/// @param decoder Decoder state
/// @return Result containing data pointer/length or error
rlp_bytes_result_t rlp_decode_bytes(rlp_decoder_t *decoder);

/// Decode a uint64 value.
/// @param decoder Decoder state
/// @return Result containing value or error
rlp_u64_result_t rlp_decode_u64(rlp_decoder_t *decoder);

/// Decode a uint256 value.
/// @param decoder Decoder state
/// @return Result containing value or error
rlp_uint256_result_t rlp_decode_uint256(rlp_decoder_t *decoder);

/// Decode an address (expects exactly 20 bytes).
/// @param decoder Decoder state
/// @return Result containing address or error
rlp_address_result_t rlp_decode_address(rlp_decoder_t *decoder);

/// Decode a list header, returning the payload length.
/// After this, call decode methods for each list item.
/// @param decoder Decoder state
/// @return Result containing payload length or error
rlp_list_result_t rlp_decode_list_header(rlp_decoder_t *decoder);

/// Skip the current item (string or list).
/// Safe even if data is truncated - clamps to buffer end.
/// @param decoder Decoder state
void rlp_skip_item(rlp_decoder_t *decoder);

/// Check if there's more data to decode.
/// @param decoder Decoder state
/// @return true if more data available
static inline bool rlp_decoder_has_more(const rlp_decoder_t *decoder) {
  return decoder->pos < decoder->size;
}

/// Get current position in input buffer.
/// @param decoder Decoder state
/// @return Current read position
static inline size_t rlp_decoder_position(const rlp_decoder_t *decoder) {
  return decoder->pos;
}

/// Get remaining bytes in input buffer.
/// @param decoder Decoder state
/// @return Remaining bytes
static inline size_t rlp_decoder_remaining(const rlp_decoder_t *decoder) {
  return decoder->size - decoder->pos;
}

/// Peek at the next prefix byte without consuming it.
/// @param decoder Decoder state
/// @return Next byte, or 0 if at end
static inline uint8_t rlp_decoder_peek(const rlp_decoder_t *decoder) {
  return decoder->pos < decoder->size ? decoder->input[decoder->pos] : 0;
}

/// Check if next item is a list.
/// @param decoder Decoder state
/// @return true if next item is a list
static inline bool rlp_decoder_next_is_list(const rlp_decoder_t *decoder) {
  return rlp_is_list_prefix(rlp_decoder_peek(decoder));
}

/// Check if next item is a string/bytes.
/// @param decoder Decoder state
/// @return true if next item is a string
static inline bool rlp_decoder_next_is_string(const rlp_decoder_t *decoder) {
  return rlp_is_string_prefix(rlp_decoder_peek(decoder));
}

/// Get human-readable error message.
/// @param error Error code
/// @return Static string describing the error
const char *rlp_error_string(rlp_error_t error);

#endif // DIV0_RLP_DECODE_H
