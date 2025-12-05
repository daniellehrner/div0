// Unit tests for hex-prefix (compact) encoding
// Test vectors from Ethereum Yellow Paper and trie tests

#include <div0/trie/hex_prefix.h>
#include <gtest/gtest.h>

#include <array>
#include <utility>

namespace div0::trie {

// =============================================================================
// hex_prefix_encode: Yellow Paper examples
// =============================================================================

TEST(HexPrefixEncode, ExtensionEvenNibbles) {
  // Extension node with even number of nibbles
  // nibbles: [1, 2, 3, 4]
  // is_leaf: false
  // Result: [0x00, 0x12, 0x34]
  const Nibbles nibbles = {0x1, 0x2, 0x3, 0x4};
  const auto result = hex_prefix_encode(nibbles, false);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0x00);
  EXPECT_EQ(result[1], 0x12);
  EXPECT_EQ(result[2], 0x34);
}

TEST(HexPrefixEncode, ExtensionOddNibbles) {
  // Extension node with odd number of nibbles
  // nibbles: [1, 2, 3]
  // is_leaf: false
  // Result: [0x11, 0x23]
  const Nibbles nibbles = {0x1, 0x2, 0x3};
  const auto result = hex_prefix_encode(nibbles, false);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0x11);
  EXPECT_EQ(result[1], 0x23);
}

TEST(HexPrefixEncode, LeafEvenNibbles) {
  // Leaf node with even number of nibbles
  // nibbles: [1, 2, 3, 4]
  // is_leaf: true
  // Result: [0x20, 0x12, 0x34]
  const Nibbles nibbles = {0x1, 0x2, 0x3, 0x4};
  const auto result = hex_prefix_encode(nibbles, true);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0x20);
  EXPECT_EQ(result[1], 0x12);
  EXPECT_EQ(result[2], 0x34);
}

TEST(HexPrefixEncode, LeafOddNibbles) {
  // Leaf node with odd number of nibbles
  // nibbles: [1, 2, 3]
  // is_leaf: true
  // Result: [0x31, 0x23]
  const Nibbles nibbles = {0x1, 0x2, 0x3};
  const auto result = hex_prefix_encode(nibbles, true);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0x31);
  EXPECT_EQ(result[1], 0x23);
}

TEST(HexPrefixEncode, EmptyExtension) {
  // Empty extension
  const Nibbles nibbles;
  const auto result = hex_prefix_encode(nibbles, false);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x00);
}

TEST(HexPrefixEncode, EmptyLeaf) {
  // Empty leaf
  const Nibbles nibbles;
  const auto result = hex_prefix_encode(nibbles, true);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x20);
}

TEST(HexPrefixEncode, SingleNibbleExtension) {
  // Single nibble extension (odd)
  const Nibbles nibbles = {0xf};
  const auto result = hex_prefix_encode(nibbles, false);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x1f);
}

TEST(HexPrefixEncode, SingleNibbleLeaf) {
  // Single nibble leaf (odd)
  const Nibbles nibbles = {0x5};
  const auto result = hex_prefix_encode(nibbles, true);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x35);
}

// =============================================================================
// hex_prefix_decode
// =============================================================================

TEST(HexPrefixDecode, ExtensionEvenNibbles) {
  const std::array<uint8_t, 3> encoded = {0x00, 0x12, 0x34};
  const auto [nibbles, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_FALSE(is_leaf);
  ASSERT_EQ(nibbles.size(), 4);
  EXPECT_EQ(nibbles[0], 0x1);
  EXPECT_EQ(nibbles[1], 0x2);
  EXPECT_EQ(nibbles[2], 0x3);
  EXPECT_EQ(nibbles[3], 0x4);
}

TEST(HexPrefixDecode, ExtensionOddNibbles) {
  const std::array<uint8_t, 2> encoded = {0x11, 0x23};
  const auto [nibbles, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_FALSE(is_leaf);
  ASSERT_EQ(nibbles.size(), 3);
  EXPECT_EQ(nibbles[0], 0x1);
  EXPECT_EQ(nibbles[1], 0x2);
  EXPECT_EQ(nibbles[2], 0x3);
}

TEST(HexPrefixDecode, LeafEvenNibbles) {
  const std::array<uint8_t, 3> encoded = {0x20, 0x12, 0x34};
  const auto [nibbles, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_TRUE(is_leaf);
  ASSERT_EQ(nibbles.size(), 4);
  EXPECT_EQ(nibbles[0], 0x1);
  EXPECT_EQ(nibbles[1], 0x2);
  EXPECT_EQ(nibbles[2], 0x3);
  EXPECT_EQ(nibbles[3], 0x4);
}

TEST(HexPrefixDecode, LeafOddNibbles) {
  const std::array<uint8_t, 2> encoded = {0x31, 0x23};
  const auto [nibbles, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_TRUE(is_leaf);
  ASSERT_EQ(nibbles.size(), 3);
  EXPECT_EQ(nibbles[0], 0x1);
  EXPECT_EQ(nibbles[1], 0x2);
  EXPECT_EQ(nibbles[2], 0x3);
}

TEST(HexPrefixDecode, EmptyExtension) {
  const std::array<uint8_t, 1> encoded = {0x00};
  const auto [nibbles, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_FALSE(is_leaf);
  EXPECT_TRUE(nibbles.empty());
}

TEST(HexPrefixDecode, EmptyLeaf) {
  const std::array<uint8_t, 1> encoded = {0x20};
  const auto [nibbles, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_TRUE(is_leaf);
  EXPECT_TRUE(nibbles.empty());
}

TEST(HexPrefixDecode, SingleNibbleExtension) {
  const std::array<uint8_t, 1> encoded = {0x1f};
  const auto [nibbles, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_FALSE(is_leaf);
  ASSERT_EQ(nibbles.size(), 1);
  EXPECT_EQ(nibbles[0], 0xf);
}

TEST(HexPrefixDecode, SingleNibbleLeaf) {
  const std::array<uint8_t, 1> encoded = {0x35};
  const auto [nibbles, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_TRUE(is_leaf);
  ASSERT_EQ(nibbles.size(), 1);
  EXPECT_EQ(nibbles[0], 0x5);
}

TEST(HexPrefixDecode, EmptyInput) {
  const std::span<const uint8_t> empty;
  const auto [nibbles, is_leaf] = hex_prefix_decode(empty);

  EXPECT_FALSE(is_leaf);
  EXPECT_TRUE(nibbles.empty());
}

// =============================================================================
// Roundtrip tests
// =============================================================================

TEST(HexPrefixRoundtrip, Extension) {
  const Nibbles original = {0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
  const auto encoded = hex_prefix_encode(original, false);
  const auto [decoded, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_FALSE(is_leaf);
  EXPECT_EQ(decoded, original);
}

TEST(HexPrefixRoundtrip, Leaf) {
  const Nibbles original = {0x1, 0x2, 0x3, 0x4, 0x5};
  const auto encoded = hex_prefix_encode(original, true);
  const auto [decoded, is_leaf] = hex_prefix_decode(encoded);

  EXPECT_TRUE(is_leaf);
  EXPECT_EQ(decoded, original);
}

TEST(HexPrefixRoundtrip, AllNibbleValues) {
  Nibbles original;
  for (uint8_t i = 0; i < 16; ++i) {
    original.push_back(i);
  }

  // Test as extension
  {
    const auto encoded = hex_prefix_encode(original, false);
    const auto [decoded, is_leaf] = hex_prefix_decode(encoded);
    EXPECT_FALSE(is_leaf);
    EXPECT_EQ(decoded, original);
  }

  // Test as leaf
  {
    const auto encoded = hex_prefix_encode(original, true);
    const auto [decoded, is_leaf] = hex_prefix_decode(encoded);
    EXPECT_TRUE(is_leaf);
    EXPECT_EQ(decoded, original);
  }
}

}  // namespace div0::trie
