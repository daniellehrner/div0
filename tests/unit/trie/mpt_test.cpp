// Unit tests for Merkle Patricia Trie
// Test vectors from Ethereum test suite:
// https://github.com/ethereum/tests/tree/develop/TrieTests

#include <div0/trie/mpt.h>
#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace div0::trie {

namespace {

// Helper to convert string to Bytes
types::Bytes to_bytes(const std::string& s) { return {s.begin(), s.end()}; }

// Helper to convert hex string to Bytes
types::Bytes hex_to_bytes(const std::string& hex) {
  types::Bytes result;
  for (size_t i = 0; i < hex.size(); i += 2) {
    const auto byte = static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16));
    result.push_back(byte);
  }
  return result;
}

}  // namespace

// =============================================================================
// Empty Trie
// =============================================================================

TEST(MerklePatriciaTrie, EmptyTrie) {
  const MerklePatriciaTrie trie;

  EXPECT_TRUE(trie.empty());
  EXPECT_EQ(trie.size(), 0);

  // Empty trie root = keccak256(RLP(empty string)) = keccak256(0x80)
  // = 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421
  const auto expected =
      types::Hash::from_hex("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
  EXPECT_EQ(trie.root_hash(), expected);
  EXPECT_EQ(trie.root_hash(), MerklePatriciaTrie::EMPTY_ROOT);
}

// =============================================================================
// Ethereum Test Vectors from trieanyorder.json
// =============================================================================

TEST(MerklePatriciaTrie, EthereumVector_SingleItem) {
  // singleItem test from trieanyorder.json
  // Key: "A", Value: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  // Root: 0xd23786fb4a010da3ce639d66d5e904a11dbc02746d1ce25029e53290cabf28ab
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("A"), to_bytes("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

  const auto expected =
      types::Hash::from_hex("d23786fb4a010da3ce639d66d5e904a11dbc02746d1ce25029e53290cabf28ab");
  EXPECT_EQ(trie.root_hash(), expected);
}

TEST(MerklePatriciaTrie, EthereumVector_Dogs) {
  // dogs test from trieanyorder.json
  // doe -> reindeer, dog -> puppy, dogglesworth -> cat
  // Root: 0x8aad789dff2f538bca5d8ea56e8abe10f4c7ba3a5dea95fea4cd6e7c3a1168d3
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("doe"), to_bytes("reindeer"));
  trie.insert(to_bytes("dog"), to_bytes("puppy"));
  trie.insert(to_bytes("dogglesworth"), to_bytes("cat"));

  const auto expected =
      types::Hash::from_hex("8aad789dff2f538bca5d8ea56e8abe10f4c7ba3a5dea95fea4cd6e7c3a1168d3");
  EXPECT_EQ(trie.root_hash(), expected);
}

TEST(MerklePatriciaTrie, EthereumVector_Puppy) {
  // puppy test from trieanyorder.json
  // do -> verb, horse -> stallion, doge -> coin, dog -> puppy
  // Root: 0x5991bb8c6514148a29db676a14ac506cd2cd5775ace63c30a4fe457715e9ac84
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("do"), to_bytes("verb"));
  trie.insert(to_bytes("horse"), to_bytes("stallion"));
  trie.insert(to_bytes("doge"), to_bytes("coin"));
  trie.insert(to_bytes("dog"), to_bytes("puppy"));

  const auto expected =
      types::Hash::from_hex("5991bb8c6514148a29db676a14ac506cd2cd5775ace63c30a4fe457715e9ac84");
  EXPECT_EQ(trie.root_hash(), expected);
}

TEST(MerklePatriciaTrie, EthereumVector_Foo) {
  // foo test from trieanyorder.json
  // foo -> bar, food -> bass
  // Root: 0x17beaa1648bafa633cda809c90c04af50fc8aed3cb40d16efbddee6fdf63c4c3
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("foo"), to_bytes("bar"));
  trie.insert(to_bytes("food"), to_bytes("bass"));

  const auto expected =
      types::Hash::from_hex("17beaa1648bafa633cda809c90c04af50fc8aed3cb40d16efbddee6fdf63c4c3");
  EXPECT_EQ(trie.root_hash(), expected);
}

TEST(MerklePatriciaTrie, EthereumVector_SmallValues) {
  // smallValues test from trieanyorder.json
  // be -> e, dog -> puppy, bed -> d
  // Root: 0x3f67c7a47520f79faa29255d2d3c084a7a6df0453116ed7232ff10277a8be68b
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("be"), to_bytes("e"));
  trie.insert(to_bytes("dog"), to_bytes("puppy"));
  trie.insert(to_bytes("bed"), to_bytes("d"));

  const auto expected =
      types::Hash::from_hex("3f67c7a47520f79faa29255d2d3c084a7a6df0453116ed7232ff10277a8be68b");
  EXPECT_EQ(trie.root_hash(), expected);
}

TEST(MerklePatriciaTrie, EthereumVector_Testy) {
  // testy test from trieanyorder.json
  // test -> test, te -> testy
  // Root: 0x8452568af70d8d140f58d941338542f645fcca50094b20f3c3d8c3df49337928
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("test"), to_bytes("test"));
  trie.insert(to_bytes("te"), to_bytes("testy"));

  const auto expected =
      types::Hash::from_hex("8452568af70d8d140f58d941338542f645fcca50094b20f3c3d8c3df49337928");
  EXPECT_EQ(trie.root_hash(), expected);
}

TEST(MerklePatriciaTrie, EthereumVector_Hex) {
  // hex test from trieanyorder.json
  // 0x0045 -> 0x0123456789, 0x4500 -> 0x9876543210
  // Root: 0x285505fcabe84badc8aa310e2aae17eddc7d120aabec8a476902c8184b3a3503
  MerklePatriciaTrie trie;
  trie.insert(hex_to_bytes("0045"), hex_to_bytes("0123456789"));
  trie.insert(hex_to_bytes("4500"), hex_to_bytes("9876543210"));

  const auto expected =
      types::Hash::from_hex("285505fcabe84badc8aa310e2aae17eddc7d120aabec8a476902c8184b3a3503");
  EXPECT_EQ(trie.root_hash(), expected);
}

// =============================================================================
// Basic Operations
// =============================================================================

TEST(MerklePatriciaTrie, InsertAndGet) {
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("key"), to_bytes("value"));

  EXPECT_EQ(trie.size(), 1);
  EXPECT_FALSE(trie.empty());

  const auto result = trie.get(to_bytes("key"));
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above
  EXPECT_EQ(*result, to_bytes("value"));
}

TEST(MerklePatriciaTrie, GetNonExistent) {
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("key"), to_bytes("value"));

  auto result = trie.get(to_bytes("other"));
  EXPECT_FALSE(result.has_value());
}

TEST(MerklePatriciaTrie, UpdateExisting) {
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("key"), to_bytes("value1"));
  trie.insert(to_bytes("key"), to_bytes("value2"));

  EXPECT_EQ(trie.size(), 1);

  const auto result = trie.get(to_bytes("key"));
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above
  EXPECT_EQ(*result, to_bytes("value2"));
}

TEST(MerklePatriciaTrie, Clear) {
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("key1"), to_bytes("value1"));
  trie.insert(to_bytes("key2"), to_bytes("value2"));

  EXPECT_EQ(trie.size(), 2);

  trie.clear();
  EXPECT_TRUE(trie.empty());
  EXPECT_EQ(trie.size(), 0);
  EXPECT_EQ(trie.root_hash(), MerklePatriciaTrie::EMPTY_ROOT);
}

// =============================================================================
// Determinism
// =============================================================================

TEST(MerklePatriciaTrie, InsertOrderIndependent) {
  // Same keys inserted in different order should give same root
  MerklePatriciaTrie trie1;
  trie1.insert(to_bytes("a"), to_bytes("1"));
  trie1.insert(to_bytes("b"), to_bytes("2"));
  trie1.insert(to_bytes("c"), to_bytes("3"));

  MerklePatriciaTrie trie2;
  trie2.insert(to_bytes("c"), to_bytes("3"));
  trie2.insert(to_bytes("a"), to_bytes("1"));
  trie2.insert(to_bytes("b"), to_bytes("2"));

  MerklePatriciaTrie trie3;
  trie3.insert(to_bytes("b"), to_bytes("2"));
  trie3.insert(to_bytes("c"), to_bytes("3"));
  trie3.insert(to_bytes("a"), to_bytes("1"));

  EXPECT_EQ(trie1.root_hash(), trie2.root_hash());
  EXPECT_EQ(trie2.root_hash(), trie3.root_hash());
}

TEST(MerklePatriciaTrie, DifferentValuesDifferentRoots) {
  MerklePatriciaTrie trie1;
  trie1.insert(to_bytes("key"), to_bytes("value1"));

  MerklePatriciaTrie trie2;
  trie2.insert(to_bytes("key"), to_bytes("value2"));

  EXPECT_NE(trie1.root_hash(), trie2.root_hash());
}

TEST(MerklePatriciaTrie, DifferentKeysDifferentRoots) {
  MerklePatriciaTrie trie1;
  trie1.insert(to_bytes("key1"), to_bytes("value"));

  MerklePatriciaTrie trie2;
  trie2.insert(to_bytes("key2"), to_bytes("value"));

  EXPECT_NE(trie1.root_hash(), trie2.root_hash());
}

// =============================================================================
// Constructor from pairs
// =============================================================================

TEST(MerklePatriciaTrie, ConstructFromPairs) {
  std::vector<std::pair<types::Bytes, types::Bytes>> items = {
      {to_bytes("doe"), to_bytes("reindeer")},
      {to_bytes("dog"), to_bytes("puppy")},
      {to_bytes("dogglesworth"), to_bytes("cat")},
  };

  const MerklePatriciaTrie trie(items);

  const auto expected =
      types::Hash::from_hex("8aad789dff2f538bca5d8ea56e8abe10f4c7ba3a5dea95fea4cd6e7c3a1168d3");
  EXPECT_EQ(trie.root_hash(), expected);
}

// =============================================================================
// Ethereum Test Vectors from trietest.json (ordered operations)
// =============================================================================

TEST(MerklePatriciaTrie, EthereumVector_InsertMiddleLeaf) {
  // insert-middle-leaf test from trietest.json
  // Tests inserting keys that share prefixes
  // Root: 0xcb65032e2f76c48b82b5c24b3db8f670ce73982869d38cd39a624f23d62a9e89
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("key1aa"), to_bytes("0123456789012345678901234567890123456789xxx"));
  trie.insert(to_bytes("key1"), to_bytes("0123456789012345678901234567890123456789Very_Long"));
  trie.insert(to_bytes("key2bb"), to_bytes("aval3"));
  trie.insert(to_bytes("key2"), to_bytes("short"));
  trie.insert(to_bytes("key3cc"), to_bytes("aval3"));
  trie.insert(to_bytes("key3"), to_bytes("1234567890123456789012345678901"));

  const auto expected =
      types::Hash::from_hex("cb65032e2f76c48b82b5c24b3db8f670ce73982869d38cd39a624f23d62a9e89");
  EXPECT_EQ(trie.root_hash(), expected);
}

TEST(MerklePatriciaTrie, EthereumVector_BranchValueUpdate) {
  // branch-value-update test from trietest.json
  // Tests updating a value at a branch point
  // Root: 0x7a320748f780ad9ad5b0837302075ce0eeba6c26e3d8562c67ccc0f1b273298a
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("abc"), to_bytes("123"));
  trie.insert(to_bytes("abcd"), to_bytes("abcd"));
  trie.insert(to_bytes("abc"), to_bytes("abc"));  // Update existing key

  const auto expected =
      types::Hash::from_hex("7a320748f780ad9ad5b0837302075ce0eeba6c26e3d8562c67ccc0f1b273298a");
  EXPECT_EQ(trie.root_hash(), expected);
}

// =============================================================================
// Additional Edge Cases
// =============================================================================

TEST(MerklePatriciaTrie, SingleByteKey) {
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("a"), to_bytes("value"));

  EXPECT_EQ(trie.size(), 1);
  const auto result = trie.get(to_bytes("a"));
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above
  EXPECT_EQ(*result, to_bytes("value"));
}

TEST(MerklePatriciaTrie, EmptyValue) {
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("key"), to_bytes(""));

  EXPECT_EQ(trie.size(), 1);
  const auto result = trie.get(to_bytes("key"));
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above
  EXPECT_TRUE((*result).empty());
}

TEST(MerklePatriciaTrie, LongKey) {
  // Test with a 32-byte key (typical for storage keys)
  const auto key = hex_to_bytes("0000000000000000000000000000000000000000000000000000000000000001");
  const auto value = to_bytes("storage_value");

  MerklePatriciaTrie trie;
  trie.insert(key, value);

  EXPECT_EQ(trie.size(), 1);
  const auto result = trie.get(key);
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above
  EXPECT_EQ(*result, value);
}

TEST(MerklePatriciaTrie, ManyKeys) {
  // Test with many keys to exercise branch nodes
  MerklePatriciaTrie trie;

  for (int i = 0; i < 100; ++i) {
    const auto key = to_bytes("key" + std::to_string(i));
    const auto value = to_bytes("value" + std::to_string(i));
    trie.insert(key, value);
  }

  EXPECT_EQ(trie.size(), 100);

  // Verify all values
  for (int i = 0; i < 100; ++i) {
    const auto k = to_bytes("key" + std::to_string(i));
    const auto expected_value = to_bytes("value" + std::to_string(i));
    const auto result = trie.get(k);
    ASSERT_TRUE(result.has_value()) << "Missing key" << i;
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above
    EXPECT_EQ(*result, expected_value) << "Wrong value for key" << i;
  }
}

TEST(MerklePatriciaTrie, SharedPrefixKeys) {
  // Keys with shared prefixes
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("a"), to_bytes("1"));
  trie.insert(to_bytes("ab"), to_bytes("2"));
  trie.insert(to_bytes("abc"), to_bytes("3"));
  trie.insert(to_bytes("abcd"), to_bytes("4"));
  trie.insert(to_bytes("abcde"), to_bytes("5"));

  EXPECT_EQ(trie.size(), 5);

  const auto r1 = trie.get(to_bytes("a"));
  const auto r2 = trie.get(to_bytes("ab"));
  const auto r3 = trie.get(to_bytes("abc"));
  const auto r4 = trie.get(to_bytes("abcd"));
  const auto r5 = trie.get(to_bytes("abcde"));
  ASSERT_TRUE(r1.has_value());
  ASSERT_TRUE(r2.has_value());
  ASSERT_TRUE(r3.has_value());
  ASSERT_TRUE(r4.has_value());
  ASSERT_TRUE(r5.has_value());
  // NOLINTBEGIN(bugprone-unchecked-optional-access) - checked above
  EXPECT_EQ(*r1, to_bytes("1"));
  EXPECT_EQ(*r2, to_bytes("2"));
  EXPECT_EQ(*r3, to_bytes("3"));
  EXPECT_EQ(*r4, to_bytes("4"));
  EXPECT_EQ(*r5, to_bytes("5"));
  // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(MerklePatriciaTrie, DivergedKeys) {
  // Keys that share a prefix then diverge
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("prefix_a"), to_bytes("value_a"));
  trie.insert(to_bytes("prefix_b"), to_bytes("value_b"));
  trie.insert(to_bytes("prefix_c"), to_bytes("value_c"));

  EXPECT_EQ(trie.size(), 3);

  const auto ra = trie.get(to_bytes("prefix_a"));
  const auto rb = trie.get(to_bytes("prefix_b"));
  const auto rc = trie.get(to_bytes("prefix_c"));
  ASSERT_TRUE(ra.has_value());
  ASSERT_TRUE(rb.has_value());
  ASSERT_TRUE(rc.has_value());
  // NOLINTBEGIN(bugprone-unchecked-optional-access) - checked above
  EXPECT_EQ(*ra, to_bytes("value_a"));
  EXPECT_EQ(*rb, to_bytes("value_b"));
  EXPECT_EQ(*rc, to_bytes("value_c"));
  // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(MerklePatriciaTrie, AllNibbleValues) {
  // Insert keys starting with each nibble value (0-15)
  MerklePatriciaTrie trie;

  for (uint8_t i = 0; i < 16; ++i) {
    const types::Bytes k = {i, 0x00};  // Key starting with nibble i
    const types::Bytes v = {i};
    trie.insert(k, v);
  }

  EXPECT_EQ(trie.size(), 16);

  for (uint8_t i = 0; i < 16; ++i) {
    const types::Bytes k = {i, 0x00};
    const types::Bytes expected_value = {i};
    const auto result = trie.get(k);
    ASSERT_TRUE(result.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above
    EXPECT_EQ(*result, expected_value);
  }
}

TEST(MerklePatriciaTrie, BinaryKeys) {
  // Test with binary (non-ASCII) keys
  MerklePatriciaTrie trie;
  trie.insert(hex_to_bytes("00"), to_bytes("zero"));
  trie.insert(hex_to_bytes("ff"), to_bytes("max"));
  trie.insert(hex_to_bytes("0f"), to_bytes("fifteen"));
  trie.insert(hex_to_bytes("f0"), to_bytes("two-forty"));

  EXPECT_EQ(trie.size(), 4);

  const auto r00 = trie.get(hex_to_bytes("00"));
  const auto rff = trie.get(hex_to_bytes("ff"));
  const auto r0f = trie.get(hex_to_bytes("0f"));
  const auto rf0 = trie.get(hex_to_bytes("f0"));
  ASSERT_TRUE(r00.has_value());
  ASSERT_TRUE(rff.has_value());
  ASSERT_TRUE(r0f.has_value());
  ASSERT_TRUE(rf0.has_value());
  // NOLINTBEGIN(bugprone-unchecked-optional-access) - checked above
  EXPECT_EQ(*r00, to_bytes("zero"));
  EXPECT_EQ(*rff, to_bytes("max"));
  EXPECT_EQ(*r0f, to_bytes("fifteen"));
  EXPECT_EQ(*rf0, to_bytes("two-forty"));
  // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(MerklePatriciaTrie, UpdateMultipleTimes) {
  // Update the same key multiple times
  MerklePatriciaTrie trie;

  trie.insert(to_bytes("key"), to_bytes("value1"));
  const auto hash1 = trie.root_hash();

  trie.insert(to_bytes("key"), to_bytes("value2"));
  const auto hash2 = trie.root_hash();

  trie.insert(to_bytes("key"), to_bytes("value3"));
  const auto hash3 = trie.root_hash();

  // All hashes should be different
  EXPECT_NE(hash1, hash2);
  EXPECT_NE(hash2, hash3);
  EXPECT_NE(hash1, hash3);

  // Final value should be "value3"
  const auto final_result = trie.get(to_bytes("key"));
  ASSERT_TRUE(final_result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above
  EXPECT_EQ(*final_result, to_bytes("value3"));
}

TEST(MerklePatriciaTrie, LargeValue) {
  // Test with a large value
  MerklePatriciaTrie trie;
  const types::Bytes large_value(1000, 0x42);  // 1000 bytes of 0x42

  trie.insert(to_bytes("key"), large_value);

  const auto result = trie.get(to_bytes("key"));
  ASSERT_TRUE(result.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - checked above
  EXPECT_EQ(*result, large_value);
}

// =============================================================================
// Root Hash Consistency
// =============================================================================

TEST(MerklePatriciaTrie, RootHashConsistentAcrossRebuild) {
  // Building the same trie twice should give the same root
  auto build_trie = []() {
    MerklePatriciaTrie trie;
    trie.insert(to_bytes("apple"), to_bytes("fruit"));
    trie.insert(to_bytes("banana"), to_bytes("yellow"));
    trie.insert(to_bytes("cherry"), to_bytes("red"));
    trie.insert(to_bytes("date"), to_bytes("sweet"));
    return trie.root_hash();
  };

  const auto hash1 = build_trie();
  const auto hash2 = build_trie();
  const auto hash3 = build_trie();

  EXPECT_EQ(hash1, hash2);
  EXPECT_EQ(hash2, hash3);
}

TEST(MerklePatriciaTrie, RootHashNonZero) {
  // Non-empty trie should never have zero hash
  MerklePatriciaTrie trie;
  trie.insert(to_bytes("key"), to_bytes("value"));

  EXPECT_FALSE(trie.root_hash().is_zero());
}

}  // namespace div0::trie
