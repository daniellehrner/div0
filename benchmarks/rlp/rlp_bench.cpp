#include <benchmark/benchmark.h>

#include <random>
#include <vector>

#include "div0/rlp/rlp.h"

using namespace div0::rlp;
using namespace div0::types;

// Fixed seed for reproducibility
static std::mt19937_64 rng(42);  // NOLINT(cert-msc51-cpp, *-avoid-non-const-global-variables)

static Uint256 random_uint256() { return Uint256(rng(), rng(), rng(), rng()); }

static std::vector<uint8_t> random_bytes(size_t len) {
  std::vector<uint8_t> result(len);
  for (auto& b : result) {
    b = static_cast<uint8_t>(rng());
  }
  return result;
}

// NOLINTBEGIN(*-owning-memory, *-avoid-non-const-global-variables)

// ============================================================================
// Encoding Benchmarks
// ============================================================================

static void bm_rlp_encode_empty(benchmark::State& state) {
  std::vector<uint8_t> buf(1);

  for ([[maybe_unused]] auto _ : state) {
    RlpEncoder encoder(buf);
    encoder.encode(std::span<const uint8_t>{});
    benchmark::DoNotOptimize(buf);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_encode_empty);

static void bm_rlp_encode_single_byte(benchmark::State& state) {
  const std::array<uint8_t, 1> data = {0x42};
  std::vector<uint8_t> buf(1);

  for ([[maybe_unused]] auto _ : state) {
    RlpEncoder encoder(buf);
    encoder.encode(data);
    benchmark::DoNotOptimize(buf);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_encode_single_byte);

static void bm_rlp_encode_short_string(benchmark::State& state) {
  const auto data = random_bytes(32);
  const size_t len = RlpEncoder::encoded_length(data);
  std::vector<uint8_t> buf(len);

  for ([[maybe_unused]] auto _ : state) {
    RlpEncoder encoder(buf);
    encoder.encode(data);
    benchmark::DoNotOptimize(buf);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * 32);
}
BENCHMARK(bm_rlp_encode_short_string);

static void bm_rlp_encode_long_string(benchmark::State& state) {
  const auto data = random_bytes(256);
  const size_t len = RlpEncoder::encoded_length(data);
  std::vector<uint8_t> buf(len);

  for ([[maybe_unused]] auto _ : state) {
    RlpEncoder encoder(buf);
    encoder.encode(data);
    benchmark::DoNotOptimize(buf);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * 256);
}
BENCHMARK(bm_rlp_encode_long_string);

static void bm_rlp_encode_uint64(benchmark::State& state) {
  const uint64_t value = rng();
  const size_t len = RlpEncoder::encoded_length(value);
  std::vector<uint8_t> buf(len);

  for ([[maybe_unused]] auto _ : state) {
    RlpEncoder encoder(buf);
    encoder.encode(value);
    benchmark::DoNotOptimize(buf);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_encode_uint64);

static void bm_rlp_encode_uint256(benchmark::State& state) {
  const Uint256 value = random_uint256();
  const size_t len = RlpEncoder::encoded_length(value);
  std::vector<uint8_t> buf(len);

  for ([[maybe_unused]] auto _ : state) {
    RlpEncoder encoder(buf);
    encoder.encode(value);
    benchmark::DoNotOptimize(buf);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_encode_uint256);

static void bm_rlp_encode_address(benchmark::State& state) {
  const auto addr_bytes = random_bytes(20);
  std::array<uint8_t, 20> fixed_bytes{};
  std::copy_n(addr_bytes.begin(), 20, fixed_bytes.begin());
  std::span<const uint8_t, 20> span{fixed_bytes};
  const Address addr{span};
  std::vector<uint8_t> buf(21);

  for ([[maybe_unused]] auto _ : state) {
    RlpEncoder encoder(buf);
    encoder.encode_address(addr);
    benchmark::DoNotOptimize(buf);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_encode_address);

// ============================================================================
// Decoding Benchmarks
// ============================================================================

static void bm_rlp_decode_empty(benchmark::State& state) {
  const std::vector<uint8_t> input = {0x80};

  for ([[maybe_unused]] auto _ : state) {
    RlpDecoder decoder(input);
    auto result = decoder.decode_bytes();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_decode_empty);

static void bm_rlp_decode_single_byte(benchmark::State& state) {
  const std::vector<uint8_t> input = {0x42};

  for ([[maybe_unused]] auto _ : state) {
    RlpDecoder decoder(input);
    auto result = decoder.decode_bytes();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_decode_single_byte);

static void bm_rlp_decode_short_string(benchmark::State& state) {
  const auto data = random_bytes(32);
  const size_t len = RlpEncoder::encoded_length(data);
  std::vector<uint8_t> encoded(len);
  RlpEncoder encoder(encoded);
  encoder.encode(data);

  for ([[maybe_unused]] auto _ : state) {
    RlpDecoder decoder(encoded);
    auto result = decoder.decode_bytes();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * 32);
}
BENCHMARK(bm_rlp_decode_short_string);

static void bm_rlp_decode_long_string(benchmark::State& state) {
  const auto data = random_bytes(256);
  const size_t len = RlpEncoder::encoded_length(data);
  std::vector<uint8_t> encoded(len);
  RlpEncoder encoder(encoded);
  encoder.encode(data);

  for ([[maybe_unused]] auto _ : state) {
    RlpDecoder decoder(encoded);
    auto result = decoder.decode_bytes();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * 256);
}
BENCHMARK(bm_rlp_decode_long_string);

static void bm_rlp_decode_uint64(benchmark::State& state) {
  const uint64_t value = rng();
  const size_t len = RlpEncoder::encoded_length(value);
  std::vector<uint8_t> encoded(len);
  RlpEncoder encoder(encoded);
  encoder.encode(value);

  for ([[maybe_unused]] auto _ : state) {
    RlpDecoder decoder(encoded);
    auto result = decoder.decode_uint64();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_decode_uint64);

static void bm_rlp_decode_uint256(benchmark::State& state) {
  const Uint256 value = random_uint256();
  const size_t len = RlpEncoder::encoded_length(value);
  std::vector<uint8_t> encoded(len);
  RlpEncoder encoder(encoded);
  encoder.encode(value);

  for ([[maybe_unused]] auto _ : state) {
    RlpDecoder decoder(encoded);
    auto result = decoder.decode_uint256();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_decode_uint256);

static void bm_rlp_decode_address(benchmark::State& state) {
  const auto addr_bytes = random_bytes(20);
  std::array<uint8_t, 20> fixed_bytes{};
  std::copy_n(addr_bytes.begin(), 20, fixed_bytes.begin());
  std::span<const uint8_t, 20> span{fixed_bytes};
  const Address addr{span};
  std::vector<uint8_t> encoded(21);
  RlpEncoder encoder(encoded);
  encoder.encode_address(addr);

  for ([[maybe_unused]] auto _ : state) {
    RlpDecoder decoder(encoded);
    auto result = decoder.decode_address();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_decode_address);

// ============================================================================
// List Benchmarks
// ============================================================================

static void bm_rlp_encode_list_header(benchmark::State& state) {
  const size_t payload_len = 100;
  const size_t header_len = RlpEncoder::list_header_length(payload_len);
  std::vector<uint8_t> buf(header_len);

  for ([[maybe_unused]] auto _ : state) {
    RlpEncoder encoder(buf);
    encoder.start_list(payload_len);
    benchmark::DoNotOptimize(buf);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_encode_list_header);

static void bm_rlp_decode_list_header(benchmark::State& state) {
  const size_t payload_len = 100;
  const size_t header_len = RlpEncoder::list_header_length(payload_len);
  std::vector<uint8_t> encoded(header_len);
  RlpEncoder encoder(encoded);
  encoder.start_list(payload_len);

  for ([[maybe_unused]] auto _ : state) {
    RlpDecoder decoder(encoded);
    auto result = decoder.decode_list_header();
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_decode_list_header);

// ============================================================================
// Length Calculation Benchmarks
// ============================================================================

static void bm_rlp_encoded_length_bytes(benchmark::State& state) {
  const auto data = random_bytes(static_cast<size_t>(state.range(0)));

  for ([[maybe_unused]] auto _ : state) {
    size_t len = RlpEncoder::encoded_length(data);
    benchmark::DoNotOptimize(len);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_encoded_length_bytes)->Arg(1)->Arg(32)->Arg(55)->Arg(56)->Arg(256)->Arg(1024);

static void bm_rlp_encoded_length_uint256(benchmark::State& state) {
  const Uint256 value = random_uint256();

  for ([[maybe_unused]] auto _ : state) {
    size_t len = RlpEncoder::encoded_length(value);
    benchmark::DoNotOptimize(len);
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_rlp_encoded_length_uint256);

// NOLINTEND(*-owning-memory, *-avoid-non-const-global-variables)