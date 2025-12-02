// Unit tests for EVM Memory implementation
// Covers: expansion, load/store operations, word alignment, and gas cost calculations

#include <div0/evm/gas/dynamic_costs.h>
#include <div0/evm/memory.h>
#include <gtest/gtest.h>

namespace div0::evm {
namespace {

using types::Uint256;

// =============================================================================
// Construction and Initial State
// =============================================================================

TEST(MemoryConstruct, DefaultConstructorCreatesEmptyMemory) {
  const Memory memory;
  EXPECT_EQ(memory.size(), 0);
  EXPECT_EQ(memory.size_words(), 0);
  EXPECT_TRUE(memory.empty());
}

TEST(MemoryConstruct, CustomCapacityDoesNotAffectSize) {
  const Memory memory(8192);
  EXPECT_EQ(memory.size(), 0);  // Size is logical, not capacity
  EXPECT_TRUE(memory.empty());
}

// =============================================================================
// Memory Expansion
// =============================================================================

TEST(MemoryExpand, ExpandToWordBoundary) {
  Memory memory;
  memory.expand(1);              // Request 1 byte
  EXPECT_EQ(memory.size(), 32);  // Should expand to full word
}

TEST(MemoryExpand, ExpandAlreadyAligned) {
  Memory memory;
  memory.expand(32);
  EXPECT_EQ(memory.size(), 32);
  EXPECT_EQ(memory.size_words(), 1);
}

TEST(MemoryExpand, ExpandMultipleWords) {
  Memory memory;
  memory.expand(65);             // 2 words + 1 byte
  EXPECT_EQ(memory.size(), 96);  // 3 words
  EXPECT_EQ(memory.size_words(), 3);
}

TEST(MemoryExpand, ExpandMonotonicallyIncreasing) {
  Memory memory;
  memory.expand(100);
  EXPECT_EQ(memory.size(), 128);  // 4 words

  memory.expand(50);              // Smaller request
  EXPECT_EQ(memory.size(), 128);  // Should not shrink
}

TEST(MemoryExpand, ExpandFromZero) {
  Memory memory;
  EXPECT_EQ(memory.size(), 0);

  memory.expand(0);
  EXPECT_EQ(memory.size(), 0);  // No expansion for zero

  memory.expand(1);
  EXPECT_EQ(memory.size(), 32);
}

TEST(MemoryExpand, ExpandLargeSize) {
  Memory memory;
  memory.expand(1000);
  EXPECT_EQ(memory.size(), 1024);  // 32 words
  EXPECT_EQ(memory.size_words(), 32);
}

// =============================================================================
// Word Alignment Calculations
// =============================================================================

TEST(MemoryAlignment, WordAlignedSizeZero) { EXPECT_EQ(Memory::word_aligned_size(0), 0); }

TEST(MemoryAlignment, WordAlignedSizeExact) {
  EXPECT_EQ(Memory::word_aligned_size(32), 32);
  EXPECT_EQ(Memory::word_aligned_size(64), 64);
  EXPECT_EQ(Memory::word_aligned_size(96), 96);
}

TEST(MemoryAlignment, WordAlignedSizeRoundsUp) {
  EXPECT_EQ(Memory::word_aligned_size(1), 32);
  EXPECT_EQ(Memory::word_aligned_size(31), 32);
  EXPECT_EQ(Memory::word_aligned_size(33), 64);
  EXPECT_EQ(Memory::word_aligned_size(63), 64);
}

TEST(MemoryAlignment, WordCountZero) { EXPECT_EQ(Memory::word_count(0), 0); }

TEST(MemoryAlignment, WordCountExact) {
  EXPECT_EQ(Memory::word_count(32), 1);
  EXPECT_EQ(Memory::word_count(64), 2);
}

TEST(MemoryAlignment, WordCountRoundsUp) {
  EXPECT_EQ(Memory::word_count(1), 1);
  EXPECT_EQ(Memory::word_count(33), 2);
  EXPECT_EQ(Memory::word_count(65), 3);
}

// =============================================================================
// Store and Load Word (32-byte operations)
// =============================================================================

TEST(MemoryWord, StoreAndLoadSimpleValue) {
  Memory memory;
  memory.expand(32);

  const auto value = Uint256(0x12345678);
  memory.store_word_unsafe(0, value);

  EXPECT_EQ(memory.load_word_unsafe(0), value);
}

TEST(MemoryWord, StoreAndLoadMaxValue) {
  Memory memory;
  memory.expand(32);

  const auto max_value = Uint256::max();
  memory.store_word_unsafe(0, max_value);

  EXPECT_EQ(memory.load_word_unsafe(0), max_value);
}

TEST(MemoryWord, StoreAndLoadZero) {
  Memory memory;
  memory.expand(32);

  // First store a non-zero value
  memory.store_word_unsafe(0, Uint256(0xDEADBEEF));

  // Then store zero
  memory.store_word_unsafe(0, Uint256::zero());

  EXPECT_EQ(memory.load_word_unsafe(0), Uint256::zero());
}

TEST(MemoryWord, StoreAndLoadMultiLimbValue) {
  Memory memory;
  memory.expand(32);

  const auto value = Uint256(0x1111111111111111ULL, 0x2222222222222222ULL, 0x3333333333333333ULL,
                             0x4444444444444444ULL);
  memory.store_word_unsafe(0, value);

  EXPECT_EQ(memory.load_word_unsafe(0), value);
}

TEST(MemoryWord, StoreAtDifferentOffsets) {
  Memory memory;
  memory.expand(128);

  const auto value1 = Uint256(111);
  const auto value2 = Uint256(222);
  const auto value3 = Uint256(333);

  memory.store_word_unsafe(0, value1);
  memory.store_word_unsafe(32, value2);
  memory.store_word_unsafe(64, value3);

  EXPECT_EQ(memory.load_word_unsafe(0), value1);
  EXPECT_EQ(memory.load_word_unsafe(32), value2);
  EXPECT_EQ(memory.load_word_unsafe(64), value3);
}

TEST(MemoryWord, StoreOverwritesPrevious) {
  Memory memory;
  memory.expand(32);

  memory.store_word_unsafe(0, Uint256(100));
  memory.store_word_unsafe(0, Uint256(200));

  EXPECT_EQ(memory.load_word_unsafe(0), Uint256(200));
}

TEST(MemoryWord, UnalignedStoreAndLoad) {
  Memory memory;
  memory.expand(64);

  // Store at unaligned offset (not a multiple of 32)
  const auto value = Uint256(0xCAFEBABE);
  memory.store_word_unsafe(16, value);

  EXPECT_EQ(memory.load_word_unsafe(16), value);
}

// =============================================================================
// Store Byte (MSTORE8)
// =============================================================================

TEST(MemoryByte, StoreSingleByte) {
  Memory memory;
  memory.expand(32);

  memory.store_byte_unsafe(0, 0x42);

  // Load word and check first byte
  const auto word = memory.load_word_unsafe(0);
  // Big-endian: byte at offset 0 is MSB of the 32-byte word
  // When loaded as Uint256, this should be in the high limb
  EXPECT_EQ(word, Uint256(0, 0, 0, 0x4200000000000000ULL));
}

TEST(MemoryByte, StoreByteAtVariousOffsets) {
  Memory memory;
  memory.expand(32);

  // Store bytes at different positions within the word
  memory.store_byte_unsafe(0, 0xAA);
  memory.store_byte_unsafe(31, 0xBB);

  auto span = memory.read_span_unsafe(0, 32);
  EXPECT_EQ(span[0], 0xAA);
  EXPECT_EQ(span[31], 0xBB);
}

TEST(MemoryByte, StoreByteOverwritesPrevious) {
  Memory memory;
  memory.expand(1);

  memory.store_byte_unsafe(0, 0x11);
  memory.store_byte_unsafe(0, 0x22);

  auto span = memory.read_span_unsafe(0, 1);
  EXPECT_EQ(span[0], 0x22);
}

// =============================================================================
// Span Access
// =============================================================================

TEST(MemorySpan, ReadSpanReturnsCorrectData) {
  Memory memory;
  memory.expand(64);

  // Store known pattern
  memory.store_word_unsafe(0, Uint256(0x1234567890ABCDEFULL, 0, 0, 0));

  auto span = memory.read_span_unsafe(24, 8);
  EXPECT_EQ(span.size(), 8);
}

TEST(MemorySpan, WriteSpanModifiesMemory) {
  Memory memory;
  memory.expand(32);

  auto span = memory.write_span_unsafe(0, 4);
  span[0] = 0xDE;
  span[1] = 0xAD;
  span[2] = 0xBE;
  span[3] = 0xEF;

  auto read_span = memory.read_span_unsafe(0, 4);
  EXPECT_EQ(read_span[0], 0xDE);
  EXPECT_EQ(read_span[1], 0xAD);
  EXPECT_EQ(read_span[2], 0xBE);
  EXPECT_EQ(read_span[3], 0xEF);
}

TEST(MemorySpan, EmptySpan) {
  Memory memory;
  memory.expand(32);

  auto span = memory.read_span_unsafe(0, 0);
  EXPECT_TRUE(span.empty());
}

// =============================================================================
// Zero Initialization
// =============================================================================

TEST(MemoryZeroInit, NewMemoryIsZeroFilled) {
  Memory memory;
  memory.expand(64);

  // All bytes should be zero
  auto span = memory.read_span_unsafe(0, 64);
  for (size_t i = 0; i < 64; ++i) {
    EXPECT_EQ(span[i], 0) << "Byte at offset " << i << " should be zero";
  }
}

TEST(MemoryZeroInit, ExpansionZeroFillsNewBytes) {
  Memory memory;
  memory.expand(32);

  // Write to first word
  memory.store_word_unsafe(0, Uint256::max());

  // Expand
  memory.expand(64);

  // Second word should be zero
  EXPECT_EQ(memory.load_word_unsafe(32), Uint256::zero());
}

// =============================================================================
// Clear
// =============================================================================

TEST(MemoryClear, ClearResetsSize) {
  Memory memory;
  memory.expand(100);
  EXPECT_GT(memory.size(), 0);

  memory.clear();
  EXPECT_EQ(memory.size(), 0);
  EXPECT_TRUE(memory.empty());
}

TEST(MemoryClear, CanExpandAfterClear) {
  Memory memory;
  memory.expand(64);
  memory.store_word_unsafe(0, Uint256(42));

  memory.clear();

  memory.expand(32);
  EXPECT_EQ(memory.size(), 32);
}

// =============================================================================
// Gas Cost Calculations
// =============================================================================

TEST(MemoryGas, CostForWordsFormula) {
  // cost = 3 * words + words² / 512
  EXPECT_EQ(gas::memory::cost_for_words(0), 0);
  EXPECT_EQ(gas::memory::cost_for_words(1), 3);    // 3 * 1 + 1/512 = 3
  EXPECT_EQ(gas::memory::cost_for_words(10), 30);  // 3 * 10 + 100/512 = 30
  EXPECT_EQ(gas::memory::cost_for_words(32), 98);  // 3 * 32 + 1024/512 = 96 + 2 = 98
}

TEST(MemoryGas, ExpansionCostNoExpansion) {
  // No cost when new_words <= current_words
  EXPECT_EQ(gas::memory::expansion_cost(10, 5), 0);
  EXPECT_EQ(gas::memory::expansion_cost(10, 10), 0);
}

TEST(MemoryGas, ExpansionCostFromZero) {
  // Expanding from 0 to 1 word
  EXPECT_EQ(gas::memory::expansion_cost(0, 1), 3);
}

TEST(MemoryGas, ExpansionCostIncremental) {
  // Cost difference between word counts
  const auto cost_1 = gas::memory::cost_for_words(1);
  const auto cost_2 = gas::memory::cost_for_words(2);
  EXPECT_EQ(gas::memory::expansion_cost(1, 2), cost_2 - cost_1);
}

TEST(MemoryGas, AccessCostZeroLength) {
  // Zero-length access has no cost
  EXPECT_EQ(gas::memory::access_cost(0, 100, 0), 0);
  EXPECT_EQ(gas::memory::access_cost(1000, 0, 0), 0);
}

TEST(MemoryGas, AccessCostNoExpansionNeeded) {
  // Access within existing memory
  EXPECT_EQ(gas::memory::access_cost(64, 0, 32), 0);
  EXPECT_EQ(gas::memory::access_cost(64, 32, 32), 0);
}

TEST(MemoryGas, AccessCostExpansionNeeded) {
  // Access beyond current memory
  const auto cost = gas::memory::access_cost(0, 0, 32);
  EXPECT_EQ(cost, 3);  // Expand to 1 word
}

TEST(MemoryGas, AccessCostOverflow) {
  // Overflow in offset + length should return UINT64_MAX
  EXPECT_EQ(gas::memory::access_cost(0, UINT64_MAX, 1), UINT64_MAX);
  EXPECT_EQ(gas::memory::access_cost(0, UINT64_MAX - 10, 100), UINT64_MAX);
}

// =============================================================================
// EVM-Specific Patterns
// =============================================================================

TEST(MemoryEVM, MLoadAtOffset64) {
  // Common pattern: MLOAD at offset 0x40 (64)
  Memory memory;
  memory.expand(96);  // 3 words

  const auto value = Uint256(0xDEADBEEFCAFEBABEULL, 0, 0, 0);
  memory.store_word_unsafe(64, value);

  EXPECT_EQ(memory.load_word_unsafe(64), value);
}

TEST(MemoryEVM, FreeMemoryPointer) {
  // EVM free memory pointer pattern at 0x40
  Memory memory;
  memory.expand(96);

  // Initial free memory pointer typically at 0x80
  memory.store_word_unsafe(64, Uint256(0x80));

  EXPECT_EQ(memory.load_word_unsafe(64), Uint256(0x80));
}

TEST(MemoryEVM, ReturnDataPattern) {
  // Storing return data at offset with size
  Memory memory;
  memory.expand(128);

  // Store 32 bytes of return data at offset 0
  const auto return_value = Uint256(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL, 0, 0);
  memory.store_word_unsafe(0, return_value);

  // Read as span for RETURN opcode
  auto span = memory.read_span_unsafe(0, 32);
  EXPECT_EQ(span.size(), 32);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(MemoryStress, ManyExpansions) {
  Memory memory;

  for (uint64_t i = 1; i <= 100; ++i) {
    memory.expand(i * 32);
    EXPECT_EQ(memory.size(), i * 32);
  }
}

TEST(MemoryStress, ManyStoresAndLoads) {
  Memory memory;
  memory.expand(1024);  // 32 words

  // Store pattern
  for (uint64_t i = 0; i < 32; ++i) {
    memory.store_word_unsafe(i * 32, Uint256(i * 100));
  }

  // Verify all values
  for (uint64_t i = 0; i < 32; ++i) {
    EXPECT_EQ(memory.load_word_unsafe(i * 32), Uint256(i * 100));
  }
}

TEST(MemoryStress, AlternatingByteAndWordStores) {
  Memory memory;
  memory.expand(64);

  // Store words and bytes interleaved
  memory.store_word_unsafe(0, Uint256(0xAAAAAAAAAAAAAAAAULL, 0, 0, 0));
  memory.store_byte_unsafe(0, 0xFF);
  memory.store_byte_unsafe(31, 0xEE);

  auto span = memory.read_span_unsafe(0, 32);
  EXPECT_EQ(span[0], 0xFF);
  EXPECT_EQ(span[31], 0xEE);
}

// =============================================================================
// Big-Endian Byte Order
// =============================================================================

TEST(MemoryEndian, WordStoredBigEndian) {
  Memory memory;
  memory.expand(32);

  // Store value with known byte pattern
  // Uint256(0x0102030405060708) has these bytes in the low limb
  const auto value = Uint256(0x0102030405060708ULL, 0, 0, 0);
  memory.store_word_unsafe(0, value);

  auto span = memory.read_span_unsafe(0, 32);

  // In EVM, values are stored big-endian:
  // MSB first, so the high bytes of the Uint256 come first in memory
  // For Uint256(0x0102030405060708, 0, 0, 0):
  // - Bytes 0-23 are zero (from limbs 3,2,1 being zero)
  // - Bytes 24-31 are 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08

  // First 24 bytes should be zero
  for (size_t i = 0; i < 24; ++i) {
    EXPECT_EQ(span[i], 0) << "Byte " << i << " should be zero";
  }

  // Last 8 bytes should be the value in big-endian
  EXPECT_EQ(span[24], 0x01);
  EXPECT_EQ(span[25], 0x02);
  EXPECT_EQ(span[26], 0x03);
  EXPECT_EQ(span[27], 0x04);
  EXPECT_EQ(span[28], 0x05);
  EXPECT_EQ(span[29], 0x06);
  EXPECT_EQ(span[30], 0x07);
  EXPECT_EQ(span[31], 0x08);
}

TEST(MemoryEndian, LoadReconstructsOriginal) {
  Memory memory;
  memory.expand(32);

  // Any value should round-trip correctly
  const auto original = Uint256(0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL,
                                0x0011223344556677ULL);
  memory.store_word_unsafe(0, original);

  EXPECT_EQ(memory.load_word_unsafe(0), original);
}

}  // namespace
}  // namespace div0::evm
