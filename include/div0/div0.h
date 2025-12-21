#ifndef DIV0_H
#define DIV0_H

/// @file div0.h
/// @brief Main umbrella header for the div0 EVM implementation.

// Version information
#define DIV0_VERSION_MAJOR 0
#define DIV0_VERSION_MINOR 1
#define DIV0_VERSION_PATCH 0

// Core types
// NOLINTNEXTLINE(misc-include-cleaner)
#include "div0/types/uint256.h"

// Memory management
// NOLINTNEXTLINE(misc-include-cleaner)
#include "div0/mem/arena.h"

// EVM components
// NOLINTNEXTLINE(misc-include-cleaner)
#include "div0/evm/evm.h"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "div0/evm/opcodes.h"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "div0/evm/stack.h"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "div0/evm/status.h"

#endif // DIV0_H