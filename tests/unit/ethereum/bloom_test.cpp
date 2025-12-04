// Unit tests for Ethereum bloom filter

#include <div0/ethereum/bloom.h>
#include <gtest/gtest.h>

#include <array>

namespace div0::ethereum {

// =============================================================================
// Basic Functionality
// =============================================================================

TEST(Bloom, DefaultConstructorIsEmpty) {
  const Bloom bloom;
  EXPECT_TRUE(bloom.is_empty());
}

TEST(Bloom, ClearMakesEmpty) {
  Bloom bloom;
  bloom.add(types::Address::zero());
  EXPECT_FALSE(bloom.is_empty());

  bloom.clear();
  EXPECT_TRUE(bloom.is_empty());
}

TEST(Bloom, EmptyFactoryMethod) {
  const auto bloom = Bloom::empty();
  EXPECT_TRUE(bloom.is_empty());
}

// =============================================================================
// Adding and Querying
// =============================================================================

TEST(Bloom, AddAddressMakesNonEmpty) {
  Bloom bloom;
  bloom.add(types::Address::zero());
  EXPECT_FALSE(bloom.is_empty());
}

TEST(Bloom, AddTopicMakesNonEmpty) {
  Bloom bloom;
  bloom.add(types::Uint256::zero());
  EXPECT_FALSE(bloom.is_empty());
}

TEST(Bloom, AddDataMakesNonEmpty) {
  Bloom bloom;
  const std::array<uint8_t, 4> data = {0x01, 0x02, 0x03, 0x04};
  bloom.add(std::span<const uint8_t>(data));
  EXPECT_FALSE(bloom.is_empty());
}

TEST(Bloom, MayContainAddedAddress) {
  Bloom bloom;
  const types::Address addr(types::Uint256(0x1234567890abcdef));
  bloom.add(addr);
  EXPECT_TRUE(bloom.may_contain(addr));
}

TEST(Bloom, MayContainAddedTopic) {
  Bloom bloom;
  const types::Uint256 topic(0xdeadbeefcafebabe);
  bloom.add(topic);
  EXPECT_TRUE(bloom.may_contain(topic));
}

TEST(Bloom, MayContainAddedData) {
  Bloom bloom;
  const std::array<uint8_t, 8> data = {0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe};
  bloom.add(std::span<const uint8_t>(data));
  EXPECT_TRUE(bloom.may_contain(std::span<const uint8_t>(data)));
}

TEST(Bloom, EmptyBloomDoesNotContainAnything) {
  const Bloom bloom;
  const types::Address addr(types::Uint256(0x12345));
  EXPECT_FALSE(bloom.may_contain(addr));
}

TEST(Bloom, MultipleItemsAllContained) {
  Bloom bloom;

  const types::Address addr1(types::Uint256(0x111));
  const types::Address addr2(types::Uint256(0x222));
  const types::Uint256 topic1(0x333);
  const types::Uint256 topic2(0x444);

  bloom.add(addr1);
  bloom.add(addr2);
  bloom.add(topic1);
  bloom.add(topic2);

  EXPECT_TRUE(bloom.may_contain(addr1));
  EXPECT_TRUE(bloom.may_contain(addr2));
  EXPECT_TRUE(bloom.may_contain(topic1));
  EXPECT_TRUE(bloom.may_contain(topic2));
}

// =============================================================================
// Bloom OR Operations
// =============================================================================

TEST(Bloom, OrWithEmptyIsIdentity) {
  Bloom bloom;
  bloom.add(types::Address::zero());

  const Bloom empty;
  bloom |= empty;

  EXPECT_TRUE(bloom.may_contain(types::Address::zero()));
}

TEST(Bloom, OrCombinesFilters) {
  Bloom bloom1;
  Bloom bloom2;

  const types::Address addr1(types::Uint256(0xaaa));
  const types::Address addr2(types::Uint256(0xbbb));

  bloom1.add(addr1);
  bloom2.add(addr2);

  bloom1 |= bloom2;

  EXPECT_TRUE(bloom1.may_contain(addr1));
  EXPECT_TRUE(bloom1.may_contain(addr2));
}

TEST(Bloom, OrOperatorCombinesFilters) {
  Bloom bloom1;
  Bloom bloom2;

  const types::Address addr1(types::Uint256(0xaaa));
  const types::Address addr2(types::Uint256(0xbbb));

  bloom1.add(addr1);
  bloom2.add(addr2);

  const Bloom combined = bloom1 | bloom2;

  EXPECT_TRUE(combined.may_contain(addr1));
  EXPECT_TRUE(combined.may_contain(addr2));
}

TEST(Bloom, OrDoesNotModifyOriginal) {
  Bloom bloom1;
  Bloom bloom2;

  bloom1.add(types::Address(types::Uint256(0x111)));
  bloom2.add(types::Address(types::Uint256(0x222)));

  [[maybe_unused]] const Bloom combined = bloom1 | bloom2;

  // bloom2 should not contain addr1
  EXPECT_FALSE(bloom2.may_contain(types::Address(types::Uint256(0x111))));
}

// =============================================================================
// Equality
// =============================================================================

TEST(Bloom, EqualityOfEmptyBlooms) {
  const Bloom bloom1;
  const Bloom bloom2;
  EXPECT_EQ(bloom1, bloom2);
}

TEST(Bloom, EqualityAfterSameAdditions) {
  Bloom bloom1;
  Bloom bloom2;

  const types::Address addr(types::Uint256(0x12345));
  bloom1.add(addr);
  bloom2.add(addr);

  EXPECT_EQ(bloom1, bloom2);
}

TEST(Bloom, InequalityAfterDifferentAdditions) {
  Bloom bloom1;
  Bloom bloom2;

  bloom1.add(types::Address(types::Uint256(0x111)));
  bloom2.add(types::Address(types::Uint256(0x222)));

  EXPECT_NE(bloom1, bloom2);
}

// =============================================================================
// Raw Access
// =============================================================================

TEST(Bloom, SpanSize) {
  const Bloom bloom;
  EXPECT_EQ(bloom.span().size(), Bloom::SIZE);
}

TEST(Bloom, DataPointerNotNull) {
  const Bloom bloom;
  EXPECT_NE(bloom.data(), nullptr);
}

TEST(Bloom, ConstructFromSpan) {
  std::array<uint8_t, Bloom::SIZE> bytes{};
  bytes[0] = 0xFF;
  bytes[255] = 0xAA;

  const Bloom bloom{std::span<const uint8_t, Bloom::SIZE>(bytes)};

  EXPECT_EQ(bloom.span()[0], 0xFF);
  EXPECT_EQ(bloom.span()[255], 0xAA);
  EXPECT_FALSE(bloom.is_empty());
}

// =============================================================================
// Known Test Vectors
// =============================================================================

// Test that the bloom filter produces consistent results with known inputs
TEST(Bloom, ConsistentHashResults) {
  Bloom bloom1;
  Bloom bloom2;

  const types::Address addr(types::Uint256(0x1234567890abcdef));

  bloom1.add(addr);
  bloom2.add(addr);

  // Same input should produce same bits
  EXPECT_EQ(bloom1, bloom2);
}

// Test that different inputs produce different bits (with high probability)
TEST(Bloom, DifferentInputsDifferentBits) {
  Bloom bloom1;
  Bloom bloom2;

  bloom1.add(types::Uint256(1));
  bloom2.add(types::Uint256(2));

  // Very unlikely to be equal with different inputs
  EXPECT_NE(bloom1, bloom2);
}

// =============================================================================
// Performance-related (basic sanity checks)
// =============================================================================

TEST(Bloom, BulkOrPerformance) {
  // Create multiple bloom filters and combine them
  constexpr int NUM_BLOOMS = 100;
  Bloom accumulated;

  for (int i = 0; i < NUM_BLOOMS; ++i) {
    Bloom bloom;
    bloom.add(types::Uint256(static_cast<uint64_t>(i)));
    accumulated |= bloom;
  }

  // All items should be contained
  for (int i = 0; i < NUM_BLOOMS; ++i) {
    EXPECT_TRUE(accumulated.may_contain(types::Uint256(static_cast<uint64_t>(i))));
  }
}

}  // namespace div0::ethereum
