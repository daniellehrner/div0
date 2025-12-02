#include <benchmark/benchmark.h>

#include "div0/crypto/secp256k1.h"

using namespace div0::crypto;
using namespace div0::types;

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// Test vector from reth - valid signature that recovers successfully
// Hash: 0xdaf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53
const Uint256 TEST_HASH(0x1ad23db92e4c8e53ULL, 0xc7ef83eadac0f279ULL, 0x2197303d7b574746ULL,
                        0xdaf5a779ae972f97ULL);

// R: 0x28ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276
const Uint256 TEST_R(0xe1590620aa636276ULL, 0x03e1a15d3c71ff63ULL, 0x2195fe5375678660ULL,
                     0x28ef61340bd939bcULL);

// S: 0x67cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83
const Uint256 TEST_S(0x297fb1966a3b6d83ULL, 0xf555c9f3dc64214bULL, 0xecb703304b3800ccULL,
                     0x67cbe9d8997f761aULL);

// Benchmark ecrecover - ECDSA signature recovery to address
static void bm_ecrecover(benchmark::State& state) {
  Secp256k1Context ctx;

  for ([[maybe_unused]] auto _ : state) {
    auto result = ctx.ecrecover(TEST_HASH, 27, TEST_R, TEST_S);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_ecrecover);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
