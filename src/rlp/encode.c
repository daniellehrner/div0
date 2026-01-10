#include "div0/rlp/encode.h"

#include <string.h>

/// Maximum header size for RLP lists (1 prefix + 8 length bytes).
static constexpr size_t RLP_MAX_HEADER_SIZE = 9;

/// Write length as big-endian bytes into buffer.
/// Returns number of bytes written.
static size_t write_length_bytes(uint8_t *const buf, const size_t len) {
  const int num_bytes = rlp_length_of_length(len);
  switch (num_bytes) {
  case 1:
    buf[0] = (uint8_t)len;
    return 1;
  case 2:
    buf[0] = (uint8_t)(len >> 8);
    buf[1] = (uint8_t)len;
    return 2;
  case 3:
    buf[0] = (uint8_t)(len >> 16);
    buf[1] = (uint8_t)(len >> 8);
    buf[2] = (uint8_t)len;
    return 3;
  case 4:
    buf[0] = (uint8_t)(len >> 24);
    buf[1] = (uint8_t)(len >> 16);
    buf[2] = (uint8_t)(len >> 8);
    buf[3] = (uint8_t)len;
    return 4;
  default:
    // For very large lengths (> 4GB)
    for (int i = num_bytes - 1; i >= 0; --i) {
      buf[num_bytes - 1 - i] = (uint8_t)(len >> (i * 8));
    }
    return (size_t)num_bytes;
  }
}

bytes_t rlp_encode_bytes(div0_arena_t *const arena, const uint8_t *const data, const size_t len) {
  bytes_t result;
  bytes_init_arena(&result, arena);

  // Single byte in [0x00, 0x7f] range: encode as itself
  if (len == 1 && data[0] <= RLP_SINGLE_BYTE_MAX) {
    bytes_reserve(&result, 1);
    bytes_append_byte(&result, data[0]);
    return result;
  }

  // Short string: 1-byte prefix
  if (len < RLP_SMALL_PREFIX_BARRIER) {
    // Safe: len < 56, so 1 + len <= 56, no overflow possible
    bytes_reserve(&result, 1 + len);
    bytes_append_byte(&result, (uint8_t)(RLP_EMPTY_STRING_BYTE + len));
    if (len > 0) {
      bytes_append(&result, data, len);
    }
    return result;
  }

  // Long string: multi-byte length prefix
  const int len_bytes = rlp_length_of_length(len);
  // Note: len_bytes <= 8. If len is near SIZE_MAX, this could overflow.
  // bytes_reserve handles allocation failure gracefully by returning early.
  bytes_reserve(&result, 1 + (size_t)len_bytes + len);
  bytes_append_byte(&result, (uint8_t)(RLP_SHORT_STRING_MAX + len_bytes));

  uint8_t len_buf[8];
  const size_t written = write_length_bytes(len_buf, len);
  bytes_append(&result, len_buf, written);
  bytes_append(&result, data, len);

  return result;
}

bytes_t rlp_encode_u64(div0_arena_t *const arena, const uint64_t value) {
  bytes_t result;
  bytes_init_arena(&result, arena);

  if (value == 0) {
    bytes_reserve(&result, 1);
    bytes_append_byte(&result, RLP_EMPTY_STRING_BYTE);
    return result;
  }

  if (value < 128) {
    bytes_reserve(&result, 1);
    bytes_append_byte(&result, (uint8_t)value);
    return result;
  }

  // Compute byte length
  const int num_bytes = rlp_byte_length_u64(value);
  // Safe: num_bytes <= 8 for uint64, so 1 + num_bytes <= 9, no overflow possible
  bytes_reserve(&result, 1 + (size_t)num_bytes);
  bytes_append_byte(&result, (uint8_t)(RLP_EMPTY_STRING_BYTE + num_bytes));

  // Write value as big-endian, skipping leading zeros
  for (int i = num_bytes - 1; i >= 0; --i) {
    bytes_append_byte(&result, (uint8_t)(value >> (i * 8)));
  }

  return result;
}

bytes_t rlp_encode_uint256(div0_arena_t *const arena, const uint256_t *const value) {
  bytes_t result;
  bytes_init_arena(&result, arena);

  if (uint256_is_zero(*value)) {
    bytes_reserve(&result, 1);
    bytes_append_byte(&result, RLP_EMPTY_STRING_BYTE);
    return result;
  }

  // Check if single byte (value < 128)
  if (value->limbs[3] == 0 && value->limbs[2] == 0 && value->limbs[1] == 0 &&
      value->limbs[0] < 128) {
    bytes_reserve(&result, 1);
    bytes_append_byte(&result, (uint8_t)value->limbs[0]);
    return result;
  }

  // Convert to big-endian bytes
  uint8_t be_bytes[32];
  uint256_to_bytes_be(*value, be_bytes);

  // Find first non-zero byte (skip leading zeros)
  size_t start = 0;
  while (start < 32 && be_bytes[start] == 0) {
    ++start;
  }

  const size_t num_bytes = 32 - start;
  // Safe: num_bytes <= 32 for uint256, so 1 + num_bytes <= 33, no overflow possible
  bytes_reserve(&result, 1 + num_bytes);
  bytes_append_byte(&result, (uint8_t)(RLP_EMPTY_STRING_BYTE + num_bytes));
  bytes_append(&result, be_bytes + start, num_bytes);

  return result;
}

bytes_t rlp_encode_address(div0_arena_t *const arena, const address_t *const addr) {
  bytes_t result;
  bytes_init_arena(&result, arena);

  // Address is always 20 bytes, prefix is 0x94 (0x80 + 20)
  bytes_reserve(&result, 21);
  bytes_append_byte(&result, 0x94);
  bytes_append(&result, addr->bytes, ADDRESS_SIZE);

  return result;
}

void rlp_list_start(rlp_list_builder_t *const builder, bytes_t *const output) {
  builder->output = output;
  builder->header_pos = output->size;

  // Reserve 9 bytes for maximum possible header size
  static constexpr uint8_t placeholder[RLP_MAX_HEADER_SIZE] = {0};
  bytes_append(output, placeholder, RLP_MAX_HEADER_SIZE);
}

void rlp_list_end(const rlp_list_builder_t *const builder) {
  bytes_t *const output = builder->output;
  const size_t header_pos = builder->header_pos;

  // Calculate payload length (everything after the reserved header space)
  // Safe: rlp_list_start() appended RLP_MAX_HEADER_SIZE bytes at header_pos,
  // so output->size >= header_pos + RLP_MAX_HEADER_SIZE by construction.
  const size_t payload_len = output->size - header_pos - RLP_MAX_HEADER_SIZE;

  // Determine actual header size
  size_t actual_header_size;
  uint8_t header[RLP_MAX_HEADER_SIZE];

  if (payload_len < RLP_SMALL_PREFIX_BARRIER) {
    // Short list: 1-byte prefix
    header[0] = (uint8_t)(RLP_EMPTY_LIST_BYTE + payload_len);
    actual_header_size = 1;
  } else {
    // Long list: prefix + length bytes
    const int len_bytes = rlp_length_of_length(payload_len);
    header[0] = (uint8_t)(RLP_SHORT_LIST_MAX + len_bytes);
    write_length_bytes(header + 1, payload_len);
    // Safe: len_bytes <= 8, so 1 + len_bytes <= 9, no overflow possible
    actual_header_size = 1 + (size_t)len_bytes;
  }

  // Write actual header at the reserved position
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(output->data + header_pos, header, actual_header_size);

  // If header is smaller than reserved space, compact the data
  if (actual_header_size < RLP_MAX_HEADER_SIZE) {
    const size_t shift = RLP_MAX_HEADER_SIZE - actual_header_size;
    const size_t payload_start = header_pos + RLP_MAX_HEADER_SIZE;

    // Move payload left to compact
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memmove(output->data + header_pos + actual_header_size, output->data + payload_start,
            payload_len);

    // Adjust size
    output->size -= shift;
  }
}
