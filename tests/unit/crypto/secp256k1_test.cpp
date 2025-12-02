// Unit tests for secp256k1 ECDSA signature recovery
// Test vectors from Ethereum transactions and the ecrecover precompile

#include <div0/crypto/secp256k1.h>
#include <gtest/gtest.h>

#include <array>
#include <thread>
#include <vector>

namespace div0::crypto {

// =============================================================================
// Basic Functionality
// =============================================================================

TEST(Secp256k1Context, ConstructDestruct) {
  // Should not throw or crash
  const Secp256k1Context ctx;
  (void)ctx;
}

TEST(Secp256k1Context, MoveConstruct) {
  Secp256k1Context ctx1;
  const Secp256k1Context ctx2(std::move(ctx1));
  // ctx2 should be usable, ctx1 should be in valid but empty state
  (void)ctx2;
}

TEST(Secp256k1Context, MoveAssign) {
  Secp256k1Context ctx1;
  Secp256k1Context ctx2;
  ctx2 = std::move(ctx1);
  // ctx2 should be usable
}

// =============================================================================
// Ecrecover Test Vectors
// =============================================================================

// Test vector from reth: known signature and expected signer
// Hash: 0xdaf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53
// R: 18515461264373351373200002665853028612451056578545711640558177340181847433846
// S: 46948507304638947509940763649030358759909902576025900602547168820602576006531
// V: 27 (recovery_id = 0)
// Expected signer: 0x9d8a62f656a8d1615c1294fd71e9cfb3e4855a4f

// Helper: Uint256 is stored little-endian (limb 0 = least significant)
// Hash: 0xdaf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53
// In limbs (little-endian): limb0 = 0x1ad23db92e4c8e53, limb1 = 0xc7ef83eadac0f279,
//                           limb2 = 0x2197303d7b574746, limb3 = 0xdaf5a779ae972f97
const types::Uint256 TEST_HASH(0x1ad23db92e4c8e53ULL, 0xc7ef83eadac0f279ULL, 0x2197303d7b574746ULL,
                               0xdaf5a779ae972f97ULL);

// R: 0x28ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276
const types::Uint256 TEST_R(0xe1590620aa636276ULL, 0x03e1a15d3c71ff63ULL, 0x2195fe5375678660ULL,
                            0x28ef61340bd939bcULL);

// S: 0x67cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83
const types::Uint256 TEST_S(0x297fb1966a3b6d83ULL, 0xf555c9f3dc64214bULL, 0xecb703304b3800ccULL,
                            0x67cbe9d8997f761aULL);

TEST(Secp256k1Context, EcrecoverRethVector) {
  const Secp256k1Context ctx;
  constexpr uint64_t v = 27;

  const auto result = ctx.ecrecover(TEST_HASH, v, TEST_R, TEST_S);
  ASSERT_TRUE(result.has_value());

  // Convert address to bytes for comparison
  constexpr std::array<uint8_t, 20> expected_bytes = {0x9d, 0x8a, 0x62, 0xf6, 0x56, 0xa8, 0xd1,
                                                      0x61, 0x5c, 0x12, 0x94, 0xfd, 0x71, 0xe9,
                                                      0xcf, 0xb3, 0xe4, 0x85, 0x5a, 0x4f};
  const types::Address expected(expected_bytes);

  EXPECT_EQ(result.value(), expected);
}

// Test with v = 28 (recovery_id = 1)
TEST(Secp256k1Context, EcrecoverV28) {
  const Secp256k1Context ctx;

  // This is a made-up test - we just verify that v=28 is handled differently than v=27
  // and produces a different (or no) result
  const auto result_27 = ctx.ecrecover(TEST_HASH, 27, TEST_R, TEST_S);
  const auto result_28 = ctx.ecrecover(TEST_HASH, 28, TEST_R, TEST_S);

  ASSERT_TRUE(result_27.has_value());
  // With wrong recovery ID, either fails or returns different address
  if (result_28.has_value()) {
    EXPECT_NE(result_27.value(), result_28.value());
  }
}

// =============================================================================
// EIP-155 Chain ID Encoding
// =============================================================================

TEST(Secp256k1Context, EcrecoverEIP155) {
  const Secp256k1Context ctx;

  // For chain_id = 1 (mainnet):
  // v = chain_id * 2 + 35 + recovery_id
  // v = 1 * 2 + 35 + 0 = 37 for recovery_id = 0
  // v = 1 * 2 + 35 + 1 = 38 for recovery_id = 1

  // v=27 (pre-EIP-155) should give same result as v=37 with chain_id=1
  const auto result_pre_eip155 = ctx.ecrecover(TEST_HASH, 27, TEST_R, TEST_S);
  const auto result_eip155 = ctx.ecrecover(TEST_HASH, 37, TEST_R, TEST_S, 1);

  ASSERT_TRUE(result_pre_eip155.has_value());
  ASSERT_TRUE(result_eip155.has_value());
  EXPECT_EQ(result_pre_eip155.value(), result_eip155.value());
}

TEST(Secp256k1Context, EcrecoverEIP155WrongChainId) {
  const Secp256k1Context ctx;

  // v=37 with wrong chain_id should fail
  const auto result =
      ctx.ecrecover(TEST_HASH, 37, TEST_R, TEST_S, 5);  // chain_id=5 expects v=45 or 46
  EXPECT_FALSE(result.has_value());
}

// =============================================================================
// Invalid Signatures
// =============================================================================

TEST(Secp256k1Context, EcrecoverInvalidV) {
  const Secp256k1Context ctx;

  const auto hash = types::Uint256(1);
  const auto r = types::Uint256(1);
  const auto s = types::Uint256(1);

  // Invalid v values
  EXPECT_FALSE(ctx.ecrecover(hash, 0, r, s).has_value());
  EXPECT_FALSE(ctx.ecrecover(hash, 26, r, s).has_value());
  EXPECT_FALSE(ctx.ecrecover(hash, 29, r, s).has_value());
  EXPECT_FALSE(ctx.ecrecover(hash, 34, r, s).has_value());
}

TEST(Secp256k1Context, EcrecoverZeroSignature) {
  const Secp256k1Context ctx;

  const auto hash = types::Uint256(1);
  const auto r = types::Uint256(0);
  const auto s = types::Uint256(0);

  // Zero r,s should fail
  EXPECT_FALSE(ctx.ecrecover(hash, 27, r, s).has_value());
}

// =============================================================================
// recover_public_key Tests
// =============================================================================

TEST(Secp256k1Context, RecoverPublicKeyInvalidRecoveryId) {
  const Secp256k1Context ctx;

  const std::array<uint8_t, 32> hash{};
  const std::array<uint8_t, 64> sig{};

  EXPECT_FALSE(ctx.recover_public_key(hash, -1, sig).has_value());
  EXPECT_FALSE(ctx.recover_public_key(hash, 4, sig).has_value());
}

// =============================================================================
// Thread Safety
// =============================================================================

TEST(Secp256k1Context, MultipleContextsConcurrent) {
  constexpr int NUM_THREADS = 4;
  constexpr int ITERATIONS = 100;

  std::vector<std::thread> threads;
  threads.reserve(NUM_THREADS);
  std::vector<bool> results(NUM_THREADS, false);

  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back([&results, i]() {
      const Secp256k1Context ctx;  // Each thread has its own context
      for (int j = 0; j < ITERATIONS; ++j) {
        const auto result = ctx.ecrecover(TEST_HASH, 27, TEST_R, TEST_S);
        if (!result.has_value()) {
          return;
        }
      }
      results[static_cast<size_t>(i)] = true;
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  for (int i = 0; i < NUM_THREADS; ++i) {
    EXPECT_TRUE(results[static_cast<size_t>(i)]) << "Thread " << i << " failed";
  }
}

}  // namespace div0::crypto
