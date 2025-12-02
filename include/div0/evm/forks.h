#ifndef DIV0_EVM_FORKS_H
#define DIV0_EVM_FORKS_H

#include <cstdint>

namespace div0::evm {

/**
 * @brief Ethereum Virtual Machine fork versions.
 *
 * Each fork introduces new opcodes, changes gas costs, or modifies EVM behavior.
 * Forks are named after the cities where Ethereum developer conferences were held.
 *
 * @note Fork selection determines which opcodes are valid and their gas costs
 * @see https://ethereum.org/en/history for complete fork history
 */
enum class Fork : uint8_t {
  Shanghai,
  Cancun,
  Prague,
};

}  // namespace div0::evm

#endif  // DIV0_EVM_FORKS_H
