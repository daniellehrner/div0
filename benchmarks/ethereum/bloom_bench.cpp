#include <benchmark/benchmark.h>

#include <vector>

#include "div0/ethereum/bloom.h"

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

using namespace div0::ethereum;

// =============================================================================
// High-Level Bloom API Benchmarks
// =============================================================================

static void bm_bloom_add_address(benchmark::State& state) {
  Bloom bloom;
  const div0::types::Address addr(div0::types::Uint256(0x1234567890abcdef));

  for ([[maybe_unused]] auto _ : state) {
    bloom.add(addr);
    benchmark::DoNotOptimize(bloom);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_bloom_add_address);

static void bm_bloom_add_topic(benchmark::State& state) {
  Bloom bloom;
  const div0::types::Uint256 topic(0xdeadbeefcafebabe);

  for ([[maybe_unused]] auto _ : state) {
    bloom.add(topic);
    benchmark::DoNotOptimize(bloom);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_bloom_add_topic);

static void bm_bloom_may_contain_hit(benchmark::State& state) {
  Bloom bloom;
  const div0::types::Address addr(div0::types::Uint256(0x1234567890abcdef));
  bloom.add(addr);

  for ([[maybe_unused]] auto _ : state) {
    bool result = bloom.may_contain(addr);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_bloom_may_contain_hit);

static void bm_bloom_may_contain_miss(benchmark::State& state) {
  Bloom bloom;
  bloom.add(div0::types::Address(div0::types::Uint256(0x111)));
  const div0::types::Address addr(div0::types::Uint256(0x222));

  for ([[maybe_unused]] auto _ : state) {
    bool result = bloom.may_contain(addr);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_bloom_may_contain_miss);

static void bm_bloom_or_operator(benchmark::State& state) {
  Bloom bloom1;
  Bloom bloom2;
  bloom1.add(div0::types::Address(div0::types::Uint256(0x111)));
  bloom2.add(div0::types::Address(div0::types::Uint256(0x222)));

  for ([[maybe_unused]] auto _ : state) {
    bloom1 |= bloom2;
    benchmark::DoNotOptimize(bloom1);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_bloom_or_operator);

static void bm_bloom_is_empty(benchmark::State& state) {
  const Bloom bloom;

  for ([[maybe_unused]] auto _ : state) {
    bool result = bloom.is_empty();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_bloom_is_empty);

// Simulate block-level bloom aggregation (combining many transaction blooms)
static void bm_bloom_block_aggregation(benchmark::State& state) {
  constexpr int TX_COUNT = 100;
  std::vector<Bloom> tx_blooms(TX_COUNT);

  // Populate each tx bloom with some entries
  for (int i = 0; i < TX_COUNT; ++i) {
    tx_blooms[i].add(div0::types::Address(div0::types::Uint256(static_cast<uint64_t>(i))));
    tx_blooms[i].add(div0::types::Uint256(static_cast<uint64_t>(i * 2)));
  }

  for ([[maybe_unused]] auto _ : state) {
    Bloom block_bloom;
    for (const auto& tx_bloom : tx_blooms) {
      block_bloom |= tx_bloom;
    }
    benchmark::DoNotOptimize(block_bloom);
  }

  state.SetItemsProcessed(state.iterations() * TX_COUNT);
}
BENCHMARK(bm_bloom_block_aggregation);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)
