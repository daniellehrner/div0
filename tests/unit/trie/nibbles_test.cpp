// Unit tests for nibble utilities

#include <div0/trie/nibbles.h>
#include <gtest/gtest.h>

#include <array>
#include <vector>

namespace div0::trie {

// =============================================================================
// bytes_to_nibbles
// =============================================================================

TEST(Nibbles, EmptyInput) {
  const auto result = bytes_to_nibbles({});
  EXPECT_TRUE(result.empty());
}

TEST(Nibbles, SingleByte) {
  const std::array<uint8_t, 1> input = {0x12};
  const auto result = bytes_to_nibbles(input);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0x1);
  EXPECT_EQ(result[1], 0x2);
}

TEST(Nibbles, MultipleByte) {
  const std::array<uint8_t, 3> input = {0xab, 0xcd, 0xef};
  const auto result = bytes_to_nibbles(input);

  ASSERT_EQ(result.size(), 6);
  EXPECT_EQ(result[0], 0xa);
  EXPECT_EQ(result[1], 0xb);
  EXPECT_EQ(result[2], 0xc);
  EXPECT_EQ(result[3], 0xd);
  EXPECT_EQ(result[4], 0xe);
  EXPECT_EQ(result[5], 0xf);
}

TEST(Nibbles, AllZeros) {
  const std::array<uint8_t, 2> input = {0x00, 0x00};
  const auto result = bytes_to_nibbles(input);

  ASSERT_EQ(result.size(), 4);
  for (const uint8_t n : result) {
    EXPECT_EQ(n, 0);
  }
}

TEST(Nibbles, AllNines) {
  const std::array<uint8_t, 2> input = {0x99, 0x99};
  const auto result = bytes_to_nibbles(input);

  ASSERT_EQ(result.size(), 4);
  for (const uint8_t n : result) {
    EXPECT_EQ(n, 9);
  }
}

TEST(Nibbles, AllFs) {
  const std::array<uint8_t, 2> input = {0xff, 0xff};
  const auto result = bytes_to_nibbles(input);

  ASSERT_EQ(result.size(), 4);
  for (const uint8_t n : result) {
    EXPECT_EQ(n, 0xf);
  }
}

// =============================================================================
// nibbles_to_bytes
// =============================================================================

TEST(NibblesToBytes, EmptyInput) {
  const Nibbles nibbles;
  const auto result = nibbles_to_bytes(nibbles);
  EXPECT_TRUE(result.empty());
}

TEST(NibblesToBytes, SingleNibblePair) {
  const Nibbles nibbles = {0x1, 0x2};
  const auto result = nibbles_to_bytes(nibbles);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x12);
}

TEST(NibblesToBytes, MultipleBytes) {
  const Nibbles nibbles = {0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
  const auto result = nibbles_to_bytes(nibbles);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0xab);
  EXPECT_EQ(result[1], 0xcd);
  EXPECT_EQ(result[2], 0xef);
}

TEST(NibblesToBytes, OddNibbles) {
  // Odd number of nibbles: last nibble is dropped
  // This is intentional as trie keys derived from bytes always have even nibbles
  const Nibbles nibbles = {0x1, 0x2, 0x3};
  const auto result = nibbles_to_bytes(nibbles);

  // Only first 2 nibbles form a complete byte
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], 0x12);
}

TEST(NibblesToBytes, SingleNibble) {
  // Single nibble cannot form a complete byte, returns empty
  const Nibbles nibbles = {0xa};
  const auto result = nibbles_to_bytes(nibbles);

  EXPECT_TRUE(result.empty());
}

// =============================================================================
// Roundtrip: bytes -> nibbles -> bytes
// =============================================================================

TEST(NibblesRoundtrip, Basic) {
  const std::array<uint8_t, 4> input = {0xde, 0xad, 0xbe, 0xef};
  const auto nibbles = bytes_to_nibbles(input);
  const auto output = nibbles_to_bytes(nibbles);

  ASSERT_EQ(output.size(), 4);
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(output.at(i), input.at(i));
  }
}

// =============================================================================
// common_prefix_length
// =============================================================================

TEST(CommonPrefixLength, BothEmpty) {
  const Nibbles a;
  const Nibbles b;
  EXPECT_EQ(common_prefix_length(a, b), 0);
}

TEST(CommonPrefixLength, OneEmpty) {
  const Nibbles a = {0x1, 0x2, 0x3};
  const Nibbles b;
  EXPECT_EQ(common_prefix_length(a, b), 0);
  EXPECT_EQ(common_prefix_length(b, a), 0);
}

TEST(CommonPrefixLength, Identical) {
  const Nibbles a = {0x1, 0x2, 0x3};
  const Nibbles b = {0x1, 0x2, 0x3};
  EXPECT_EQ(common_prefix_length(a, b), 3);
}

TEST(CommonPrefixLength, NoCommon) {
  const Nibbles a = {0x1, 0x2, 0x3};
  const Nibbles b = {0x4, 0x5, 0x6};
  EXPECT_EQ(common_prefix_length(a, b), 0);
}

TEST(CommonPrefixLength, PartialMatch) {
  const Nibbles a = {0x1, 0x2, 0x3, 0x4};
  const Nibbles b = {0x1, 0x2, 0x5, 0x6};
  EXPECT_EQ(common_prefix_length(a, b), 2);
}

TEST(CommonPrefixLength, OnePrefixOfOther) {
  const Nibbles a = {0x1, 0x2};
  const Nibbles b = {0x1, 0x2, 0x3, 0x4};
  EXPECT_EQ(common_prefix_length(a, b), 2);
  EXPECT_EQ(common_prefix_length(b, a), 2);
}

// =============================================================================
// nibbles_slice
// =============================================================================

TEST(NibblesSlice, Empty) {
  const Nibbles nibbles;
  const auto result = nibbles_slice(nibbles, 0, 0);
  EXPECT_TRUE(result.empty());
}

TEST(NibblesSlice, Full) {
  const Nibbles nibbles = {0x1, 0x2, 0x3, 0x4};
  const auto result = nibbles_slice(nibbles, 0, 4);
  EXPECT_EQ(result, nibbles);
}

TEST(NibblesSlice, Prefix) {
  const Nibbles nibbles = {0x1, 0x2, 0x3, 0x4};
  const auto result = nibbles_slice(nibbles, 0, 2);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0x1);
  EXPECT_EQ(result[1], 0x2);
}

TEST(NibblesSlice, Suffix) {
  const Nibbles nibbles = {0x1, 0x2, 0x3, 0x4};
  const auto result = nibbles_slice(nibbles, 2, 2);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0x3);
  EXPECT_EQ(result[1], 0x4);
}

TEST(NibblesSlice, Middle) {
  const Nibbles nibbles = {0x1, 0x2, 0x3, 0x4, 0x5};
  const auto result = nibbles_slice(nibbles, 1, 3);

  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], 0x2);
  EXPECT_EQ(result[1], 0x3);
  EXPECT_EQ(result[2], 0x4);
}

TEST(NibblesSlice, ClampsToBounds) {
  const Nibbles nibbles = {0x1, 0x2, 0x3};
  // Request more than available
  const auto result = nibbles_slice(nibbles, 1, 100);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], 0x2);
  EXPECT_EQ(result[1], 0x3);
}

TEST(NibblesSlice, StartBeyondEnd) {
  const Nibbles nibbles = {0x1, 0x2, 0x3};
  const auto result = nibbles_slice(nibbles, 10, 2);
  EXPECT_TRUE(result.empty());
}

}  // namespace div0::trie
