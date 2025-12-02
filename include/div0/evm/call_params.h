#ifndef DIV0_EVM_CALL_PARAMS_H
#define DIV0_EVM_CALL_PARAMS_H

#include <cstdint>
#include <span>

#include "div0/types/address.h"
#include "div0/types/uint256.h"

namespace div0::evm {

/**
 * @brief Initial call parameters for EVM execution.
 *
 * These values populate the first CallFrame. For nested calls (CALL, CREATE),
 * the EVM creates new CallFrames with different parameters.
 */
struct CallParams {
  // Fields ordered for optimal packing (minimizes padding)
  types::Uint256 value;            // CALLVALUE opcode (0x34) - 32 bytes
  uint64_t gas;                    // Gas available - 8 bytes
  std::span<const uint8_t> code;   // Code to execute (CODESIZE/COPY) - 16 bytes
  std::span<const uint8_t> input;  // Input data (CALLDATALOAD/SIZE/COPY) - 16 bytes
  types::Address caller;           // CALLER opcode (0x33) - 24 bytes
  types::Address address;          // ADDRESS opcode (0x30) - 24 bytes
  bool is_static;                  // STATICCALL flag - prohibits state modifications - 1 byte
};

}  // namespace div0::evm

#endif  // DIV0_EVM_CALL_PARAMS_H
