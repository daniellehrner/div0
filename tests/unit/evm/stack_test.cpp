// Unit tests for EVM Stack implementation
// Covers: push, pop, dup, swap, indexing, size management, and boundary conditions
// Tests follow EVM specification where stack depth is 1024 max and uses LIFO semantics

#include <div0/evm/stack.h>
#include <gtest/gtest.h>

namespace div0::evm {
namespace {

using types::Uint256;

// Maximum uint64 value for convenience
constexpr uint64_t MAX64 = UINT64_MAX;

// =============================================================================
// Push: Basic Operations
// =============================================================================

TEST(StackPush, PushSingleValue) {
  Stack stack;
  EXPECT_TRUE(stack.push(Uint256(42)));
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack.top(), Uint256(42));
}

TEST(StackPush, PushMultipleValues) {
  Stack stack;
  EXPECT_TRUE(stack.push(Uint256(100)));
  EXPECT_TRUE(stack.push(Uint256(200)));
  EXPECT_TRUE(stack.push(Uint256(300)));

  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack[0], Uint256(300));  // top
  EXPECT_EQ(stack[1], Uint256(200));
  EXPECT_EQ(stack[2], Uint256(100));  // bottom
}

TEST(StackPush, PushZero) {
  Stack stack;
  EXPECT_TRUE(stack.push(Uint256(0)));
  EXPECT_EQ(stack.top(), Uint256(0));
}

TEST(StackPush, PushMaxValue) {
  Stack stack;
  constexpr auto MAX256 = Uint256(MAX64, MAX64, MAX64, MAX64);
  EXPECT_TRUE(stack.push(MAX256));
  EXPECT_EQ(stack.top(), MAX256);
}

TEST(StackPush, PushMultiLimbValues) {
  Stack stack;
  constexpr auto VALUE = Uint256(0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0x87654321);
  EXPECT_TRUE(stack.push(VALUE));
  EXPECT_EQ(stack.top(), VALUE);
}

// =============================================================================
// Push: Overflow Handling
// =============================================================================

TEST(StackPush, OverflowAtMaxSize) {
  Stack stack;

  // Fill stack to capacity
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    EXPECT_TRUE(stack.push(Uint256(i)));
  }

  EXPECT_EQ(stack.size(), Stack::MAX_SIZE);

  // Next push should fail
  EXPECT_FALSE(stack.push(Uint256(9999)));
  EXPECT_EQ(stack.size(), Stack::MAX_SIZE);  // Size unchanged
}

TEST(StackPush, OverflowDoesNotCorruptData) {
  Stack stack;

  // Fill stack
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  // Failed push shouldn't corrupt top
  const auto original_top = stack.top();
  EXPECT_FALSE(stack.push(Uint256(0xBADBAD)));
  EXPECT_EQ(stack.top(), original_top);
}

// =============================================================================
// Pop: Basic Operations
// =============================================================================

TEST(StackPop, PopSingleValue) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  EXPECT_TRUE(stack.pop());
  EXPECT_EQ(stack.size(), 0);
  EXPECT_TRUE(stack.empty());
}

TEST(StackPop, PopMultipleValues) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  EXPECT_TRUE(stack.pop());
  EXPECT_EQ(stack.size(), 2);
  EXPECT_EQ(stack.top(), Uint256(200));

  EXPECT_TRUE(stack.pop());
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack.top(), Uint256(100));

  EXPECT_TRUE(stack.pop());
  EXPECT_EQ(stack.size(), 0);
}

TEST(StackPop, PopReturnsCorrectTopAfter) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));
  ASSERT_TRUE(stack.push(Uint256(2)));
  ASSERT_TRUE(stack.push(Uint256(3)));

  ASSERT_TRUE(stack.pop());
  EXPECT_EQ(stack.top(), Uint256(2));

  ASSERT_TRUE(stack.pop());
  EXPECT_EQ(stack.top(), Uint256(1));
}

// =============================================================================
// Pop: Underflow Handling
// =============================================================================

TEST(StackPop, UnderflowOnEmptyStack) {
  Stack stack;
  EXPECT_FALSE(stack.pop());
  EXPECT_TRUE(stack.empty());
}

TEST(StackPop, UnderflowAfterPushPop) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));
  EXPECT_TRUE(stack.pop());
  EXPECT_FALSE(stack.pop());  // Should fail now
}

TEST(StackPop, MultipleUnderflowAttempts) {
  Stack stack;
  EXPECT_FALSE(stack.pop());
  EXPECT_FALSE(stack.pop());
  EXPECT_FALSE(stack.pop());
  EXPECT_TRUE(stack.empty());
}

// =============================================================================
// PopUnsafe: Unsafe Pop Operations
// =============================================================================

TEST(StackPopUnsafe, ReturnsTopValue) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  auto& value = stack.pop_unsafe();
  EXPECT_EQ(value, Uint256(42));
  EXPECT_TRUE(stack.empty());
}

TEST(StackPopUnsafe, ReturnsCorrectValueInSequence) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  EXPECT_EQ(stack.pop_unsafe(), Uint256(300));
  EXPECT_EQ(stack.pop_unsafe(), Uint256(200));
  EXPECT_EQ(stack.pop_unsafe(), Uint256(100));
}

TEST(StackPopUnsafe, ReturnsReference) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  auto& ref = stack.pop_unsafe();
  // The reference points to memory that was "logically" removed
  // but still accessible (this is the unsafe part)
  ref = Uint256(999);  // We can modify it
  EXPECT_EQ(ref, Uint256(999));
}

// =============================================================================
// Indexing: Operator[] Access
// =============================================================================

TEST(StackIndexing, IndexZeroIsTop) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  EXPECT_EQ(stack[0], Uint256(300));
}

TEST(StackIndexing, IndexFromTop) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(10)));
  ASSERT_TRUE(stack.push(Uint256(20)));
  ASSERT_TRUE(stack.push(Uint256(30)));
  ASSERT_TRUE(stack.push(Uint256(40)));
  ASSERT_TRUE(stack.push(Uint256(50)));

  EXPECT_EQ(stack[0], Uint256(50));  // Most recent
  EXPECT_EQ(stack[1], Uint256(40));
  EXPECT_EQ(stack[2], Uint256(30));
  EXPECT_EQ(stack[3], Uint256(20));
  EXPECT_EQ(stack[4], Uint256(10));  // Oldest
}

TEST(StackIndexing, ModifyThroughIndex) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));

  stack[0] = Uint256(999);
  stack[1] = Uint256(888);

  EXPECT_EQ(stack[0], Uint256(999));
  EXPECT_EQ(stack[1], Uint256(888));
}

TEST(StackIndexing, ConstCorrectness) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  const auto& const_stack = stack;
  EXPECT_EQ(const_stack[0], Uint256(42));
}

TEST(StackIndexing, IndexAfterPopChanges) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  EXPECT_EQ(stack[0], Uint256(300));

  ASSERT_TRUE(stack.pop());
  EXPECT_EQ(stack[0], Uint256(200));  // New top

  ASSERT_TRUE(stack.pop());
  EXPECT_EQ(stack[0], Uint256(100));  // New top
}

// =============================================================================
// Top: Access Top Element
// =============================================================================

TEST(StackTop, ReturnsMostRecent) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));
  EXPECT_EQ(stack.top(), Uint256(42));

  ASSERT_TRUE(stack.push(Uint256(99)));
  EXPECT_EQ(stack.top(), Uint256(99));
}

TEST(StackTop, ModifyThroughTop) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  stack.top() = Uint256(999);
  EXPECT_EQ(stack.top(), Uint256(999));
  EXPECT_EQ(stack[0], Uint256(999));
}

TEST(StackTop, ConstCorrectness) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  const auto& const_stack = stack;
  EXPECT_EQ(const_stack.top(), Uint256(42));
}

TEST(StackTop, ConsistentWithIndexZero) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  EXPECT_EQ(stack.top(), stack[0]);
}

// =============================================================================
// Dup: Duplicate Operations (EVM DUP1-DUP16)
// =============================================================================

TEST(StackDup, Dup1DuplicatesTop) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  stack.dup_unsafe(1);
  EXPECT_EQ(stack.size(), 2);
  EXPECT_EQ(stack[0], Uint256(42));
  EXPECT_EQ(stack[1], Uint256(42));
}

TEST(StackDup, Dup2DuplicatesSecond) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));

  stack.dup_unsafe(2);
  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack[0], Uint256(100));  // Copy of second
  EXPECT_EQ(stack[1], Uint256(200));  // Original top
  EXPECT_EQ(stack[2], Uint256(100));  // Original second
}

TEST(StackDup, DupAllDepths) {
  // Test DUP1 through DUP16
  for (size_t depth = 1; depth <= 16; ++depth) {
    Stack stack;

    // Push enough values
    for (size_t i = 0; i < depth; ++i) {
      ASSERT_TRUE(stack.push(Uint256(i * 100)));
    }

    const auto value_at_depth = stack[depth - 1];
    const auto original_size = stack.size();

    stack.dup_unsafe(depth);
    EXPECT_EQ(stack.size(), original_size + 1);
    EXPECT_EQ(stack[0], value_at_depth);  // Top is now the duplicated value
  }
}

TEST(StackDup, DupPreservesOriginalValues) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  stack.dup_unsafe(2);  // Duplicate 200

  EXPECT_EQ(stack[0], Uint256(200));  // New top (duplicate)
  EXPECT_EQ(stack[1], Uint256(300));  // Original top
  EXPECT_EQ(stack[2], Uint256(200));  // Original second
  EXPECT_EQ(stack[3], Uint256(100));  // Original third
}

// Note: dup_unsafe uses asserts for preconditions, so underflow/overflow
// testing is done at the opcode handler level (opcode_dup_test.cpp)

// =============================================================================
// Swap: Swap Operations (EVM SWAP1-SWAP16)
// =============================================================================

TEST(StackSwap, Swap1SwapsTopTwo) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));

  EXPECT_TRUE(stack.swap(1));
  EXPECT_EQ(stack[0], Uint256(100));
  EXPECT_EQ(stack[1], Uint256(200));
}

TEST(StackSwap, Swap2SwapsTopAndThird) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  EXPECT_TRUE(stack.swap(2));
  EXPECT_EQ(stack[0], Uint256(100));  // Was third, now top
  EXPECT_EQ(stack[1], Uint256(200));  // Unchanged
  EXPECT_EQ(stack[2], Uint256(300));  // Was top, now third
}

TEST(StackSwap, SwapAllDepths) {
  // Test SWAP1 through SWAP16
  for (size_t depth = 1; depth <= 16; ++depth) {
    Stack stack;

    // Push enough values (need depth + 1 elements for SWAPn)
    for (size_t i = 0; i <= depth; ++i) {
      ASSERT_TRUE(stack.push(Uint256(i * 100)));
    }

    const auto top_before = stack[0];
    const auto target_before = stack[depth];

    EXPECT_TRUE(stack.swap(depth));
    EXPECT_EQ(stack[0], target_before);
    EXPECT_EQ(stack[depth], top_before);
  }
}

TEST(StackSwap, SwapPreservesSizeAndMiddleElements) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));
  ASSERT_TRUE(stack.push(Uint256(400)));
  ASSERT_TRUE(stack.push(Uint256(500)));

  const auto original_size = stack.size();

  ASSERT_TRUE(stack.swap(4));  // Swap top (500) with fifth (100)

  EXPECT_EQ(stack.size(), original_size);
  EXPECT_EQ(stack[0], Uint256(100));  // Swapped from bottom
  EXPECT_EQ(stack[1], Uint256(400));  // Unchanged
  EXPECT_EQ(stack[2], Uint256(300));  // Unchanged
  EXPECT_EQ(stack[3], Uint256(200));  // Unchanged
  EXPECT_EQ(stack[4], Uint256(500));  // Swapped from top
}

TEST(StackSwap, DoubleSwapRestoresOriginal) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  ASSERT_TRUE(stack.swap(2));
  ASSERT_TRUE(stack.swap(2));

  EXPECT_EQ(stack[0], Uint256(300));
  EXPECT_EQ(stack[1], Uint256(200));
  EXPECT_EQ(stack[2], Uint256(100));
}

// =============================================================================
// Swap: Error Handling
// =============================================================================

TEST(StackSwap, FailsOnUnderflow) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  // SWAP1 requires at least 2 elements
  EXPECT_FALSE(stack.swap(1));
  EXPECT_EQ(stack[0], Uint256(42));  // Unchanged
}

TEST(StackSwap, FailsWhenDepthExceedsAvailable) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  // SWAP3 requires 4 elements, we have 3
  EXPECT_FALSE(stack.swap(3));
}

TEST(StackSwap, DoesNotCorruptOnFailure) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));

  EXPECT_FALSE(stack.swap(5));
  EXPECT_EQ(stack[0], Uint256(200));
  EXPECT_EQ(stack[1], Uint256(100));
}

// =============================================================================
// Size: Size Tracking
// =============================================================================

TEST(StackSize, EmptyStackHasSizeZero) {
  const Stack stack;
  EXPECT_EQ(stack.size(), 0);
}

TEST(StackSize, SizeIncreasesOnPush) {
  Stack stack;

  for (size_t i = 1; i <= 10; ++i) {
    EXPECT_TRUE(stack.push(Uint256(i)));
    EXPECT_EQ(stack.size(), i);
  }
}

TEST(StackSize, SizeDecreasesOnPop) {
  Stack stack;
  for (size_t i = 0; i < 10; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  for (size_t i = 10; i > 0; --i) {
    EXPECT_EQ(stack.size(), i);
    EXPECT_TRUE(stack.pop());
  }
  EXPECT_EQ(stack.size(), 0);
}

TEST(StackSize, SizeIncreasesOnDup) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  EXPECT_EQ(stack.size(), 1);
  stack.dup_unsafe(1);
  EXPECT_EQ(stack.size(), 2);
}

TEST(StackSize, SizeUnchangedOnSwap) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));

  EXPECT_EQ(stack.size(), 2);
  EXPECT_TRUE(stack.swap(1));
  EXPECT_EQ(stack.size(), 2);
}

TEST(StackSize, MaxSizeConstant) { EXPECT_EQ(Stack::MAX_SIZE, 1024); }

// =============================================================================
// Empty: Emptiness Check
// =============================================================================

TEST(StackEmpty, NewStackIsEmpty) {
  const Stack stack;
  EXPECT_TRUE(stack.empty());
}

TEST(StackEmpty, NotEmptyAfterPush) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));
  EXPECT_FALSE(stack.empty());
}

TEST(StackEmpty, EmptyAfterAllPops) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));

  ASSERT_TRUE(stack.pop());
  EXPECT_FALSE(stack.empty());

  ASSERT_TRUE(stack.pop());
  EXPECT_TRUE(stack.empty());
}

TEST(StackEmpty, ConsistentWithSizeZero) {
  Stack stack;
  // NOLINTNEXTLINE(readability-container-size-empty) - intentionally testing size() vs empty()
  EXPECT_EQ(stack.empty(), (stack.size() == 0));

  ASSERT_TRUE(stack.push(Uint256(42)));
  // NOLINTNEXTLINE(readability-container-size-empty) - intentionally testing size() vs empty()
  EXPECT_EQ(stack.empty(), (stack.size() == 0));
}

// =============================================================================
// HasSpace: Capacity Checks
// =============================================================================

TEST(StackHasSpace, EmptyStackHasMaxSpace) {
  const Stack stack;
  EXPECT_TRUE(stack.has_space(Stack::MAX_SIZE));
  EXPECT_FALSE(stack.has_space(Stack::MAX_SIZE + 1));
}

TEST(StackHasSpace, PartiallyFilledStack) {
  Stack stack;
  for (size_t i = 0; i < 100; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  EXPECT_TRUE(stack.has_space(Stack::MAX_SIZE - 100));
  EXPECT_TRUE(stack.has_space(1));
  EXPECT_FALSE(stack.has_space(Stack::MAX_SIZE - 99));
}

TEST(StackHasSpace, FullStackHasNoSpace) {
  Stack stack;
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  EXPECT_FALSE(stack.has_space(1));
  EXPECT_TRUE(stack.has_space(0));
}

TEST(StackHasSpace, ZeroAlwaysSucceeds) {
  Stack stack;
  EXPECT_TRUE(stack.has_space(0));

  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }
  EXPECT_TRUE(stack.has_space(0));
}

// =============================================================================
// HasItems: Element Count Checks
// =============================================================================

TEST(StackHasItems, EmptyStackHasNoItems) {
  const Stack stack;
  EXPECT_FALSE(stack.has_items(1));
  EXPECT_TRUE(stack.has_items(0));
}

TEST(StackHasItems, PartiallyFilledStack) {
  Stack stack;
  for (size_t i = 0; i < 50; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  EXPECT_TRUE(stack.has_items(50));
  EXPECT_TRUE(stack.has_items(1));
  EXPECT_FALSE(stack.has_items(51));
}

TEST(StackHasItems, ExactCount) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  EXPECT_TRUE(stack.has_items(3));
  EXPECT_FALSE(stack.has_items(4));
}

TEST(StackHasItems, ZeroAlwaysSucceeds) {
  Stack stack;
  EXPECT_TRUE(stack.has_items(0));

  ASSERT_TRUE(stack.push(Uint256(42)));
  EXPECT_TRUE(stack.has_items(0));
}

// =============================================================================
// Clear: Reset Stack
// =============================================================================

TEST(StackClear, ClearsToEmpty) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  ASSERT_TRUE(stack.push(Uint256(200)));
  ASSERT_TRUE(stack.push(Uint256(300)));

  stack.clear();

  EXPECT_TRUE(stack.empty());
  EXPECT_EQ(stack.size(), 0);
}

TEST(StackClear, CanPushAfterClear) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(100)));
  stack.clear();

  EXPECT_TRUE(stack.push(Uint256(999)));
  EXPECT_EQ(stack.top(), Uint256(999));
  EXPECT_EQ(stack.size(), 1);
}

TEST(StackClear, ClearFullStack) {
  Stack stack;
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  stack.clear();
  EXPECT_TRUE(stack.empty());
  EXPECT_TRUE(stack.has_space(Stack::MAX_SIZE));
}

TEST(StackClear, MultipleClears) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));
  stack.clear();
  stack.clear();  // Second clear should be safe
  EXPECT_TRUE(stack.empty());
}

// =============================================================================
// LIFO Semantics: Verify Last-In-First-Out Behavior
// =============================================================================

TEST(StackLIFO, PushPopOrder) {
  Stack stack;
  constexpr size_t COUNT = 100;

  for (size_t i = 0; i < COUNT; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  // Pop should return in reverse order
  for (size_t i = COUNT; i > 0; --i) {
    EXPECT_EQ(stack.top(), Uint256(i - 1));
    ASSERT_TRUE(stack.pop());
  }
}

TEST(StackLIFO, InterleavedOperations) {
  Stack stack;

  ASSERT_TRUE(stack.push(Uint256(1)));
  ASSERT_TRUE(stack.push(Uint256(2)));
  EXPECT_EQ(stack.top(), Uint256(2));

  ASSERT_TRUE(stack.pop());
  EXPECT_EQ(stack.top(), Uint256(1));

  ASSERT_TRUE(stack.push(Uint256(3)));
  ASSERT_TRUE(stack.push(Uint256(4)));
  EXPECT_EQ(stack.top(), Uint256(4));

  ASSERT_TRUE(stack.pop());
  ASSERT_TRUE(stack.pop());
  EXPECT_EQ(stack.top(), Uint256(1));
}

// =============================================================================
// EVM-Specific Test Cases
// =============================================================================

TEST(StackEVM, EVMDup1Example) {
  // EVM DUP1: Duplicate top of stack
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(0xDEADBEEF)));

  stack.dup_unsafe(1);
  EXPECT_EQ(stack.size(), 2);
  EXPECT_EQ(stack[0], Uint256(0xDEADBEEF));
  EXPECT_EQ(stack[1], Uint256(0xDEADBEEF));
}

TEST(StackEVM, EVMSwap1Example) {
  // EVM SWAP1: Exchange top two elements
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(0xAAAA)));
  ASSERT_TRUE(stack.push(Uint256(0xBBBB)));

  EXPECT_TRUE(stack.swap(1));
  EXPECT_EQ(stack[0], Uint256(0xAAAA));
  EXPECT_EQ(stack[1], Uint256(0xBBBB));
}

TEST(StackEVM, EVMPushPopSequence) {
  // Simulate: PUSH1 0x60, PUSH1 0x40, MSTORE (which pops 2)
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(0x60)));
  ASSERT_TRUE(stack.push(Uint256(0x40)));

  // MSTORE would pop both - simulate
  auto value = stack.pop_unsafe();
  auto offset = stack.pop_unsafe();

  EXPECT_EQ(value, Uint256(0x40));
  EXPECT_EQ(offset, Uint256(0x60));
  EXPECT_TRUE(stack.empty());
}

TEST(StackEVM, EVMArithmeticPattern) {
  // Simulate: PUSH 10, PUSH 20, ADD
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(10)));
  ASSERT_TRUE(stack.push(Uint256(20)));

  // ADD pops two, pushes result
  auto b = stack.pop_unsafe();
  auto a = stack.pop_unsafe();
  ASSERT_TRUE(stack.push(a + b));

  EXPECT_EQ(stack.top(), Uint256(30));
  EXPECT_EQ(stack.size(), 1);
}

TEST(StackEVM, EVMDupChain) {
  // Test sequence of DUP operations
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  // DUP1, DUP1, DUP1 - should create 4 copies of 42
  stack.dup_unsafe(1);
  stack.dup_unsafe(1);
  stack.dup_unsafe(1);

  EXPECT_EQ(stack.size(), 4);
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(stack[i], Uint256(42));
  }
}

TEST(StackEVM, EVMSwapChain) {
  // Test sequence of SWAP operations
  // Stack grows upward: push 1,2,3,4 results in [4,3,2,1] (top to bottom)
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(1)));
  ASSERT_TRUE(stack.push(Uint256(2)));
  ASSERT_TRUE(stack.push(Uint256(3)));
  ASSERT_TRUE(stack.push(Uint256(4)));

  // Stack is now: [4,3,2,1] (top=4, bottom=1)
  // SWAP3 exchanges stack[0] (4) with stack[3] (1)
  ASSERT_TRUE(stack.swap(3));  // [4,3,2,1] -> [1,3,2,4]
  EXPECT_EQ(stack[0], Uint256(1));
  EXPECT_EQ(stack[3], Uint256(4));

  // SWAP1 exchanges stack[0] (1) with stack[1] (3)
  ASSERT_TRUE(stack.swap(1));  // [1,3,2,4] -> [3,1,2,4]
  EXPECT_EQ(stack[0], Uint256(3));
  EXPECT_EQ(stack[1], Uint256(1));
}

// =============================================================================
// Boundary Tests
// =============================================================================

TEST(StackBoundary, SingleElementOperations) {
  Stack stack;
  ASSERT_TRUE(stack.push(Uint256(42)));

  // DUP1 on single element
  stack.dup_unsafe(1);
  EXPECT_EQ(stack.size(), 2);

  ASSERT_TRUE(stack.pop());

  // SWAP1 fails on single element
  EXPECT_FALSE(stack.swap(1));
}

TEST(StackBoundary, MaxDepthDup) {
  Stack stack;

  // Push 16 elements for DUP16
  for (size_t i = 1; i <= 16; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i * 100)));
  }

  stack.dup_unsafe(16);  // DUP16 should work
  EXPECT_EQ(stack.size(), 17);
  EXPECT_EQ(stack[0], Uint256(100));  // Duplicated from depth 16
}

TEST(StackBoundary, MaxDepthSwap) {
  Stack stack;

  // Push 17 elements for SWAP16
  for (size_t i = 1; i <= 17; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i * 100)));
  }

  const auto top_before = stack[0];    // 1700
  const auto deep_before = stack[16];  // 100

  EXPECT_TRUE(stack.swap(16));
  EXPECT_EQ(stack[0], deep_before);
  EXPECT_EQ(stack[16], top_before);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(StackStress, FillAndEmpty) {
  Stack stack;

  // Fill completely
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    EXPECT_TRUE(stack.push(Uint256(i)));
  }
  EXPECT_EQ(stack.size(), Stack::MAX_SIZE);

  // Empty completely
  for (size_t i = 0; i < Stack::MAX_SIZE; ++i) {
    EXPECT_TRUE(stack.pop());
  }
  EXPECT_TRUE(stack.empty());
}

TEST(StackStress, RepeatedFillClear) {
  Stack stack;

  for (size_t cycle = 0; cycle < 5; ++cycle) {
    // Fill partially
    for (size_t i = 0; i < 500; ++i) {
      ASSERT_TRUE(stack.push(Uint256(i + (cycle * 1000))));
    }
    EXPECT_EQ(stack.size(), 500);

    // Clear
    stack.clear();
    EXPECT_TRUE(stack.empty());
  }
}

TEST(StackStress, ManyDupSwapOperations) {
  Stack stack;

  // Setup initial values
  for (size_t i = 1; i <= 10; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i)));
  }

  // Many DUP and SWAP operations (rely on stack initialisation with 10 items)
  for (int i = 0; i < 100; ++i) {
    if (stack.size() < Stack::MAX_SIZE - 1) {
      stack.dup_unsafe((i % 9) + 1);  // DUP1-DUP9
    }
    if (stack.size() > 5) {
      (void)stack.swap((i % 4) + 1);  // SWAP1-SWAP4
    }
  }

  // Just verify we didn't crash and size is reasonable
  EXPECT_GT(stack.size(), 10);
}

TEST(StackStress, AlternatingPushPop) {
  Stack stack;

  // Push 2, pop 1 pattern
  for (size_t i = 0; i < 100; ++i) {
    ASSERT_TRUE(stack.push(Uint256(i * 2)));
    ASSERT_TRUE(stack.push(Uint256((i * 2) + 1)));
    ASSERT_TRUE(stack.pop());
  }

  EXPECT_EQ(stack.size(), 100);
}

// =============================================================================
// Large Value Tests
// =============================================================================

TEST(StackLargeValues, Max256BitValues) {
  Stack stack;
  constexpr auto MAX256 = Uint256(MAX64, MAX64, MAX64, MAX64);

  ASSERT_TRUE(stack.push(MAX256));
  ASSERT_TRUE(stack.push(MAX256 - Uint256(1)));

  EXPECT_EQ(stack[0], MAX256 - Uint256(1));
  EXPECT_EQ(stack[1], MAX256);
}

TEST(StackLargeValues, DupPreservesLargeValues) {
  Stack stack;
  constexpr auto LARGE = Uint256(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL,
                                 0xDEADBEEFCAFEBABEULL, 0x0102030405060708ULL);

  ASSERT_TRUE(stack.push(LARGE));
  stack.dup_unsafe(1);

  EXPECT_EQ(stack[0], LARGE);
  EXPECT_EQ(stack[1], LARGE);
}

TEST(StackLargeValues, SwapPreservesLargeValues) {
  Stack stack;
  constexpr auto A = Uint256(MAX64, 0, MAX64, 0);
  constexpr auto B = Uint256(0, MAX64, 0, MAX64);

  ASSERT_TRUE(stack.push(A));
  ASSERT_TRUE(stack.push(B));
  ASSERT_TRUE(stack.swap(1));

  EXPECT_EQ(stack[0], A);
  EXPECT_EQ(stack[1], B);
}

}  // namespace
}  // namespace div0::evm
