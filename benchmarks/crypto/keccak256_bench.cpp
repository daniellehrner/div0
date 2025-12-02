#include <benchmark/benchmark.h>

#include <array>

#include "div0/crypto/keccak256.h"

using namespace div0::crypto;

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// 32 bytes - single EVM word (storage values, block hashes)
static void bm_keccak256_32bytes(benchmark::State& state) {
  std::array<uint8_t, 32> input = {};
  input[0] = 0x42;

  for ([[maybe_unused]] auto _ : state) {
    auto result = keccak256(std::span<const uint8_t>(input));
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * 32);
}
BENCHMARK(bm_keccak256_32bytes);

// 64 bytes - mapping storage slot: keccak256(key || slot)
static void bm_keccak256_64bytes(benchmark::State& state) {
  std::array<uint8_t, 64> input = {};
  input[31] = 0x01;  // key
  input[63] = 0x02;  // slot

  for ([[maybe_unused]] auto _ : state) {
    auto result = keccak256(std::span<const uint8_t>(input));
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * 64);
}
BENCHMARK(bm_keccak256_64bytes);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
