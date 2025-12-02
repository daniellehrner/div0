// Unit tests for Keccak-256 cryptographic hash function
// Test vectors from official Ethereum test suite:
// https://github.com/ethereum/tests/blob/develop/src/GeneralStateTestsFiller/VMTests/vmTests/sha3Filler.yml

#include <div0/crypto/keccak256.h>
#include <gtest/gtest.h>

#include <array>
#include <vector>

namespace div0::crypto {

// =============================================================================
// Ethereum Test Vectors: Zero-Filled Inputs
// =============================================================================
// These vectors are from the official Ethereum test suite (sha3Filler.yml)
// They hash zero-initialized memory regions of various sizes

TEST(Keccak256, EthereumVector_Empty) {
  // sha3(0, 0) - empty input
  // Hash: 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
  // This constant appears throughout Ethereum (empty code hash, empty trie, etc.)
  const auto result = keccak256(std::span<const uint8_t>{});

  constexpr auto EXPECTED = types::Uint256(0x7bfad8045d85a470ULL,  // limb 0 (least significant)
                                           0xe500b653ca82273bULL,  // limb 1
                                           0x927e7db2dcc703c0ULL,  // limb 2
                                           0xc5d2460186f7233cULL   // limb 3 (most significant)
  );

  EXPECT_EQ(result, EXPECTED);
}

TEST(Keccak256, EthereumVector_SingleZero) {
  // sha3(offset, 1) where memory[offset] = 0x00
  // Hash: 0xbc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a
  const std::array<uint8_t, 1> input = {0x00};
  const auto result = keccak256(std::span<const uint8_t>(input));

  constexpr auto EXPECTED = types::Uint256(0xff96a9e064bcc98aULL, 0x6612f7b477d66591ULL,
                                           0x36464229828f817dULL, 0xbc36789e7a1e2814ULL);

  EXPECT_EQ(result, EXPECTED);
}

TEST(Keccak256, EthereumVector_FiveZeros) {
  // sha3(4, 5) - 5 bytes of zeros
  // Hash: 0xc41589e7559804ea4a2080dad19d876a024ccb05117835447d72ce08c1d020ec
  const std::array<uint8_t, 5> input = {0x00, 0x00, 0x00, 0x00, 0x00};
  const auto result = keccak256(std::span<const uint8_t>(input));

  constexpr auto EXPECTED = types::Uint256(0x7d72ce08c1d020ecULL, 0x024ccb0511783544ULL,
                                           0x4a2080dad19d876aULL, 0xc41589e7559804eaULL);

  EXPECT_EQ(result, EXPECTED);
}

TEST(Keccak256, EthereumVector_TenZeros) {
  // sha3(10, 10) - 10 bytes of zeros
  // Hash: 0x6bd2dd6bd408cbee33429358bf24fdc64612fbf8b1b4db604518f40ffd34b607
  const std::array<uint8_t, 10> input = {};  // Zero-initialized
  const auto result = keccak256(std::span<const uint8_t>(input));

  constexpr auto EXPECTED = types::Uint256(0x4518f40ffd34b607ULL, 0x4612fbf8b1b4db60ULL,
                                           0x33429358bf24fdc6ULL, 0x6bd2dd6bd408cbeeULL);

  EXPECT_EQ(result, EXPECTED);
}

TEST(Keccak256, EthereumVector_32Zeros) {
  // sha3(2016, 32) - 32 bytes of zeros
  // Hash: 0x290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563
  // Common case: hashing a single 256-bit word
  const std::array<uint8_t, 32> input = {};  // Zero-initialized
  const auto result = keccak256(std::span<const uint8_t>(input));

  constexpr auto EXPECTED = types::Uint256(0x362f93160ef3e563ULL, 0x4ba6bc95484008f6ULL,
                                           0xd60345a988386fc8ULL, 0x290decd9548b62a8ULL);

  EXPECT_EQ(result, EXPECTED);
}

TEST(Keccak256, EthereumVector_LargeInput) {
  // sha3(1000, 0xFFFFF) - 1,048,575 bytes of zeros
  // Hash: 0xbe6f1b42b34644f918560a07f959d23e532dea5338e4b9f63db0caeb608018fa
  // Tests incremental hashing across many Keccak blocks (rate = 136 bytes)
  std::vector<uint8_t> input(0xFFFFF, 0x00);
  const auto result = keccak256(std::span<const uint8_t>(input));

  constexpr auto EXPECTED = types::Uint256(0x3db0caeb608018faULL, 0x532dea5338e4b9f6ULL,
                                           0x18560a07f959d23eULL, 0xbe6f1b42b34644f9ULL);

  EXPECT_EQ(result, EXPECTED);
}

// =============================================================================
// One-Shot API: Additional Coverage
// =============================================================================

TEST(Keccak256, NonZeroSingleByte) {
  // Verify different byte values produce different hashes
  const std::array<uint8_t, 1> input_00 = {0x00};
  const std::array<uint8_t, 1> input_ff = {0xff};
  const std::array<uint8_t, 1> input_42 = {0x42};

  const auto hash_00 = keccak256(std::span<const uint8_t>(input_00));
  const auto hash_ff = keccak256(std::span<const uint8_t>(input_ff));
  const auto hash_42 = keccak256(std::span<const uint8_t>(input_42));

  EXPECT_NE(hash_00, hash_ff);
  EXPECT_NE(hash_00, hash_42);
  EXPECT_NE(hash_ff, hash_42);
}

TEST(Keccak256, Deterministic) {
  // Same input must always produce same output
  std::vector<uint8_t> input(256);
  for (size_t i = 0; i < input.size(); ++i) {
    input[i] = static_cast<uint8_t>(i);
  }

  const auto result1 = keccak256(std::span<const uint8_t>(input));
  const auto result2 = keccak256(std::span<const uint8_t>(input));
  const auto result3 = keccak256(std::span<const uint8_t>(input));

  EXPECT_EQ(result1, result2);
  EXPECT_EQ(result2, result3);
}

TEST(Keccak256, AvalancheEffect) {
  // Changing a single bit should drastically change the hash
  std::array<uint8_t, 32> input1 = {};
  std::array<uint8_t, 32> input2 = {};
  input2[0] = 0x01;  // Flip one bit

  const auto hash1 = keccak256(std::span<const uint8_t>(input1));
  const auto hash2 = keccak256(std::span<const uint8_t>(input2));

  EXPECT_NE(hash1, hash2);

  // Verify 32 zeros matches known Ethereum vector
  constexpr auto EXPECTED_32_ZEROS = types::Uint256(0x362f93160ef3e563ULL, 0x4ba6bc95484008f6ULL,
                                                    0xd60345a988386fc8ULL, 0x290decd9548b62a8ULL);
  EXPECT_EQ(hash1, EXPECTED_32_ZEROS);
}

// =============================================================================
// Incremental API: Keccak256Hasher
// =============================================================================

TEST(Keccak256Hasher, EmptyFinalize) {
  Keccak256Hasher hasher;
  const auto result = hasher.finalize();

  // Must match one-shot empty input
  constexpr auto EXPECTED_EMPTY = types::Uint256(0x7bfad8045d85a470ULL, 0xe500b653ca82273bULL,
                                                 0x927e7db2dcc703c0ULL, 0xc5d2460186f7233cULL);

  EXPECT_EQ(result, EXPECTED_EMPTY);
}

TEST(Keccak256Hasher, SingleUpdate) {
  Keccak256Hasher hasher;
  const std::array<uint8_t, 1> input = {0x00};
  hasher.update(std::span<const uint8_t>(input));
  const auto result = hasher.finalize();

  // Must match one-shot single zero
  constexpr auto EXPECTED = types::Uint256(0xff96a9e064bcc98aULL, 0x6612f7b477d66591ULL,
                                           0x36464229828f817dULL, 0xbc36789e7a1e2814ULL);

  EXPECT_EQ(result, EXPECTED);
}

TEST(Keccak256Hasher, MultipleUpdates) {
  // Hash 10 zeros in chunks of 5
  Keccak256Hasher hasher;
  const std::array<uint8_t, 5> chunk = {};  // 5 zeros

  hasher.update(std::span<const uint8_t>(chunk));
  hasher.update(std::span<const uint8_t>(chunk));

  const auto result = hasher.finalize();

  // Must match one-shot 10 zeros
  constexpr auto EXPECTED = types::Uint256(0x4518f40ffd34b607ULL, 0x4612fbf8b1b4db60ULL,
                                           0x33429358bf24fdc6ULL, 0x6bd2dd6bd408cbeeULL);

  EXPECT_EQ(result, EXPECTED);
}

TEST(Keccak256Hasher, ByteByByteUpdate) {
  // Hash 5 zeros one byte at a time
  Keccak256Hasher hasher;
  const uint8_t zero = 0x00;

  for (int i = 0; i < 5; ++i) {
    hasher.update(std::span<const uint8_t>(&zero, 1));
  }

  const auto result = hasher.finalize();

  // Must match one-shot 5 zeros
  constexpr auto EXPECTED = types::Uint256(0x7d72ce08c1d020ecULL, 0x024ccb0511783544ULL,
                                           0x4a2080dad19d876aULL, 0xc41589e7559804eaULL);

  EXPECT_EQ(result, EXPECTED);
}

TEST(Keccak256Hasher, ReuseAfterFinalize) {
  Keccak256Hasher hasher;

  // First hash: empty
  const auto result1 = hasher.finalize();

  // Second hash: single zero (hasher auto-resets after finalize)
  const std::array<uint8_t, 1> input = {0x00};
  hasher.update(std::span<const uint8_t>(input));
  const auto result2 = hasher.finalize();

  // Third hash: empty again
  const auto result3 = hasher.finalize();

  EXPECT_NE(result1, result2);
  EXPECT_EQ(result1, result3);
}

TEST(Keccak256Hasher, ExplicitReset) {
  Keccak256Hasher hasher;

  // Start hashing something
  const std::array<uint8_t, 100> garbage = {};
  hasher.update(std::span<const uint8_t>(garbage));

  // Reset before finalize
  hasher.reset();

  // Now hash single zero
  const std::array<uint8_t, 1> input = {0x00};
  hasher.update(std::span<const uint8_t>(input));
  const auto result = hasher.finalize();

  // Should match clean single-zero hash
  constexpr auto EXPECTED = types::Uint256(0xff96a9e064bcc98aULL, 0x6612f7b477d66591ULL,
                                           0x36464229828f817dULL, 0xbc36789e7a1e2814ULL);

  EXPECT_EQ(result, EXPECTED);
}

// =============================================================================
// Consistency: One-Shot vs Incremental
// =============================================================================

TEST(Keccak256, OneShotMatchesIncremental) {
  // Test with various input sizes around Keccak block boundary (136 bytes)
  for (const size_t size : {1UL, 32UL, 64UL, 135UL, 136UL, 137UL, 256UL, 272UL, 1000UL}) {
    std::vector<uint8_t> input(size);
    for (size_t i = 0; i < size; ++i) {
      input.at(i) = static_cast<uint8_t>(i & 0xFF);
    }

    // One-shot
    const auto one_shot = keccak256(std::span<const uint8_t>(input));

    // Incremental with various chunk sizes
    for (const size_t chunk_size : {1UL, 7UL, 32UL, 64UL, 128UL}) {
      Keccak256Hasher hasher;
      for (size_t offset = 0; offset < input.size(); offset += chunk_size) {
        const size_t remaining = input.size() - offset;
        const size_t this_chunk = std::min(chunk_size, remaining);
        hasher.update(std::span<const uint8_t>(&input[offset], this_chunk));
      }
      const auto incremental = hasher.finalize();

      EXPECT_EQ(one_shot, incremental) << "Mismatch: size=" << size << ", chunk=" << chunk_size;
    }
  }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(Keccak256, EmptyUpdate) {
  Keccak256Hasher hasher;

  // Empty updates should be no-ops
  hasher.update(std::span<const uint8_t>{});

  std::vector<uint8_t> empty_vec;
  hasher.update(std::span<const uint8_t>(empty_vec));

  const auto result = hasher.finalize();

  // Should still be empty hash
  constexpr auto EXPECTED_EMPTY = types::Uint256(0x7bfad8045d85a470ULL, 0xe500b653ca82273bULL,
                                                 0x927e7db2dcc703c0ULL, 0xc5d2460186f7233cULL);

  EXPECT_EQ(result, EXPECTED_EMPTY);
}

TEST(Keccak256, ArrayTemplateOverload) {
  // Test the template overload for std::array
  const std::array<uint8_t, 10> input = {};
  const auto result = keccak256(input);

  // Should match 10 zeros
  constexpr auto EXPECTED = types::Uint256(0x4518f40ffd34b607ULL, 0x4612fbf8b1b4db60ULL,
                                           0x33429358bf24fdc6ULL, 0x6bd2dd6bd408cbeeULL);

  EXPECT_EQ(result, EXPECTED);
}

TEST(Keccak256, BlockBoundaryInputs) {
  // Keccak-256 rate is 136 bytes - test inputs at exact boundaries
  for (const size_t size : {135UL, 136UL, 137UL, 271UL, 272UL, 273UL}) {
    std::vector<uint8_t> input(size, 0x42);

    const auto result1 = keccak256(std::span<const uint8_t>(input));
    const auto result2 = keccak256(std::span<const uint8_t>(input));

    EXPECT_EQ(result1, result2) << "Non-deterministic for size " << size;
    EXPECT_NE(result1, types::Uint256(0)) << "Zero hash for size " << size;
  }
}

}  // namespace div0::crypto
