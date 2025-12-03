#include "div0/rlp/encode.h"

#include <bit>

#include "div0/utils/bytes.h"

namespace div0::rlp {

size_t RlpEncoder::encoded_length(const types::Uint256& value) noexcept {
  if (value.is_zero()) {
    return 1;  // 0x80 (empty string)
  }

  // Check limbs from most significant to find the top non-zero limb
  // Uint256 stores limbs in little-endian order: limb(0) is least significant
  uint64_t top_limb = 0;
  int base_size = 0;

  if (value.limb(3) != 0) {
    top_limb = value.limb(3);
    base_size = 1 + 32;  // prefix + up to 32 bytes
  } else if (value.limb(2) != 0) {
    top_limb = value.limb(2);
    base_size = 1 + 24;  // prefix + up to 24 bytes
  } else if (value.limb(1) != 0) {
    top_limb = value.limb(1);
    base_size = 1 + 16;  // prefix + up to 16 bytes
  } else if (value.limb(0) < 128) {
    return 1;  // Single byte, no prefix needed
  } else {
    top_limb = value.limb(0);
    base_size = 1 + 8;  // prefix + up to 8 bytes
  }

  // Subtract leading zero bytes from the top limb
  const int leading_zero_bytes = std::countl_zero(top_limb) / 8;
  return static_cast<size_t>(base_size - leading_zero_bytes);
}

void RlpEncoder::write_length_bytes(size_t len) noexcept {
  // Write length as big-endian bytes, skipping leading zeros
  switch (const int num_bytes = length_of_length(len)) {
    case 1:
      write_byte(static_cast<uint8_t>(len));
      break;
    case 2:
      write_byte(static_cast<uint8_t>(len >> 8));
      write_byte(static_cast<uint8_t>(len));
      break;
    case 3:
      write_byte(static_cast<uint8_t>(len >> 16));
      write_byte(static_cast<uint8_t>(len >> 8));
      write_byte(static_cast<uint8_t>(len));
      break;
    case 4:
      write_byte(static_cast<uint8_t>(len >> 24));
      write_byte(static_cast<uint8_t>(len >> 16));
      write_byte(static_cast<uint8_t>(len >> 8));
      write_byte(static_cast<uint8_t>(len));
      break;
    default:
      // For very large lengths (> 4GB), handle all 8 bytes
      for (int i = num_bytes - 1; i >= 0; --i) {
        write_byte(static_cast<uint8_t>(len >> (i * 8)));
      }
      break;
  }
}

void RlpEncoder::encode(std::span<const uint8_t> data) {
  const size_t len = data.size();

  // Single byte in [0x00, 0x7f] range: encode as itself
  if (len == 1 && data[0] <= SINGLE_BYTE_MAX) {
    write_byte(data[0]);
    return;
  }

  // Short string: 1-byte prefix
  if (len < SMALL_PREFIX_BARRIER) {
    write_byte(static_cast<uint8_t>(EMPTY_STRING_BYTE + len));
    write_bytes(data);
    return;
  }

  // Long string: multi-byte length prefix
  const int len_bytes = length_of_length(len);
  write_byte(static_cast<uint8_t>(SHORT_STRING_MAX + len_bytes));
  write_length_bytes(len);
  write_bytes(data);
}

void RlpEncoder::encode(uint64_t value) {
  if (value == 0) {
    write_byte(EMPTY_STRING_BYTE);  // 0x80 = empty string
    return;
  }

  if (value < 128) {
    write_byte(static_cast<uint8_t>(value));
    return;
  }

  // Compute byte length using leading zero count
  const int leading_zeros = std::countl_zero(value);
  const int num_bytes = 8 - (leading_zeros / 8);

  // Write prefix
  write_byte(static_cast<uint8_t>(EMPTY_STRING_BYTE + num_bytes));

  // Write value as big-endian, skipping leading zeros
  const auto be_bytes = std::bit_cast<std::array<uint8_t, 8>>(__builtin_bswap64(value));
  write_bytes(std::span{be_bytes}.subspan(static_cast<size_t>(8 - num_bytes)));
}

void RlpEncoder::encode(const types::Uint256& value) {
  if (value.is_zero()) {
    write_byte(EMPTY_STRING_BYTE);
    return;
  }

  // Check if single byte (value < 128)
  if (value.limb(3) == 0 && value.limb(2) == 0 && value.limb(1) == 0 && value.limb(0) < 128) {
    write_byte(static_cast<uint8_t>(value.limb(0)));
    return;
  }

  // Convert limbs to big-endian bytes using swap_endian_256
  const auto be_limbs = utils::swap_endian_256(value.data_unsafe());
  const auto be_bytes = std::bit_cast<std::array<uint8_t, 32>>(be_limbs);

  // Find first non-zero byte (skip leading zeros)
  size_t start = 0;
  while (start < 32 && be_bytes.at(start) == 0) {
    ++start;
  }

  const size_t num_bytes = 32 - start;

  // Write prefix and data
  write_byte(static_cast<uint8_t>(EMPTY_STRING_BYTE + num_bytes));
  write_bytes(std::span{be_bytes}.subspan(start));
}

void RlpEncoder::encode_address(const types::Address& addr) {
  // Address is always 20 bytes, prefix is 0x94 (0x80 + 20)
  write_byte(0x94);

  // Address stores limbs in little-endian order
  // limb[2] has upper 32 bits (bytes 0-3 in big-endian output)
  // limb[1] has middle 64 bits (bytes 4-11)
  // limb[0] has lower 64 bits (bytes 12-19)
  std::array<uint8_t, 20> bytes{};

  // Upper 4 bytes from limb[2] (only lower 32 bits used)
  const auto upper = static_cast<uint32_t>(addr.limb(2));
  bytes[0] = static_cast<uint8_t>(upper >> 24);
  bytes[1] = static_cast<uint8_t>(upper >> 16);
  bytes[2] = static_cast<uint8_t>(upper >> 8);
  bytes[3] = static_cast<uint8_t>(upper);

  // Middle 8 bytes from limb[1]
  const uint64_t mid = __builtin_bswap64(addr.limb(1));
  std::memcpy(&bytes[4], &mid, 8);

  // Lower 8 bytes from limb[0]
  const uint64_t low = __builtin_bswap64(addr.limb(0));
  std::memcpy(&bytes[12], &low, 8);

  write_bytes(bytes);
}

void RlpEncoder::start_list(const size_t payload_length) {
  if (payload_length < SMALL_PREFIX_BARRIER) {
    write_byte(static_cast<uint8_t>(EMPTY_LIST_BYTE + payload_length));
    return;
  }

  const int len_bytes = length_of_length(payload_length);
  write_byte(static_cast<uint8_t>(SHORT_LIST_MAX + len_bytes));
  write_length_bytes(payload_length);
}

}  // namespace div0::rlp
