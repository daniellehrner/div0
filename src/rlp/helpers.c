#include "div0/rlp/helpers.h"

// Pre-computed lookup table for prefix lengths.
// Indexed by prefix byte, returns header length:
// - 0x00-0x7F (single byte): 0 (no header)
// - 0x80-0xB7 (short string): 1 (1-byte prefix)
// - 0xB8-0xBF (long string): 2-9 (1 + length bytes)
// - 0xC0-0xF7 (short list): 1 (1-byte prefix)
// - 0xF8-0xFF (long list): 2-9 (1 + length bytes)
// clang-format off
const uint8_t RLP_PREFIX_LENGTH_TABLE[256] = {
  // 0x00-0x0F: single byte (no header)
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0x10-0x1F: single byte
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0x20-0x2F: single byte
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0x30-0x3F: single byte
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0x40-0x4F: single byte
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0x50-0x5F: single byte
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0x60-0x6F: single byte
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0x70-0x7F: single byte
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  // 0x80-0x8F: short string (1-byte prefix)
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 0x90-0x9F: short string
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 0xA0-0xAF: short string
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 0xB0-0xB7: short string
  1, 1, 1, 1, 1, 1, 1, 1,
  // 0xB8-0xBF: long string (1 + n bytes where n = prefix - 0xB7)
  2, 3, 4, 5, 6, 7, 8, 9,
  // 0xC0-0xCF: short list (1-byte prefix)
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 0xD0-0xDF: short list
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 0xE0-0xEF: short list
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 0xF0-0xF7: short list
  1, 1, 1, 1, 1, 1, 1, 1,
  // 0xF8-0xFF: long list (1 + n bytes where n = prefix - 0xF7)
  2, 3, 4, 5, 6, 7, 8, 9,
};
// clang-format on
