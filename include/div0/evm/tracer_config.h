#ifndef DIV0_EVM_TRACER_CONFIG_H
#define DIV0_EVM_TRACER_CONFIG_H

namespace div0::evm {

/**
 * @brief Configuration for tracer output options.
 *
 * Controls which optional fields are included in trace output.
 * These correspond to the optional fields in EIP-3155 but are also
 * applicable to other tracer implementations.
 */
struct TracerConfig {
  bool memory = false;       // Include memory contents in traces
  bool stack = true;         // Include stack contents in traces
  bool storage = false;      // Include storage changes in traces
  bool return_data = false;  // Include return data in traces
};

}  // namespace div0::evm

#endif  // DIV0_EVM_TRACER_CONFIG_H
