#ifndef DIV0_STATE_CODE_PROVIDER_H
#define DIV0_STATE_CODE_PROVIDER_H

#include <span>

#include "div0/types/address.h"

namespace div0::state {

/**
 * @brief Abstract interface for contract code access.
 *
 * Provides code retrieval for CALL, EXTCODECOPY, EXTCODESIZE, etc.
 */
class CodeProvider {
 public:
  CodeProvider() = default;
  virtual ~CodeProvider() = default;

  // non copyable, non moveable
  CodeProvider(const CodeProvider& other) = delete;
  CodeProvider(CodeProvider&& other) = delete;
  CodeProvider& operator=(const CodeProvider& other) = delete;
  CodeProvider& operator=(CodeProvider&& other) = delete;

  /// Get code for an address. Returns empty span if no code exists.
  [[nodiscard]] virtual std::span<const uint8_t> get_code(const types::Address& address) = 0;

  /// Get code size for an address. Returns 0 if no code exists.
  [[nodiscard]] virtual size_t get_code_size(const types::Address& address) = 0;
};

}  // namespace div0::state

#endif  // DIV0_STATE_CODE_PROVIDER_H
