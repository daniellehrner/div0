// fork_parser.h - Fork name parsing utilities

#ifndef DIV0_CLI_T8N_FORK_PARSER_H
#define DIV0_CLI_T8N_FORK_PARSER_H

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

#include "div0/evm/forks.h"

namespace div0::cli {

/// Parse fork name string to Fork enum (case-insensitive)
[[nodiscard]] inline std::optional<evm::Fork> parse_fork(const std::string_view name) {
  // Convert to lowercase for case-insensitive comparison
  std::string lower;
  lower.reserve(name.size());
  for (const char c : name) {
    lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }

  if (lower == "shanghai") {
    return evm::Fork::Shanghai;
  }
  if (lower == "cancun") {
    return evm::Fork::Cancun;
  }
  if (lower == "prague") {
    return evm::Fork::Prague;
  }

  return std::nullopt;
}

/// Get fork name as string
[[nodiscard]] inline std::string_view fork_name(const evm::Fork fork) {
  switch (fork) {
    case evm::Fork::Shanghai:
      return "Shanghai";
    case evm::Fork::Cancun:
      return "Cancun";
    case evm::Fork::Prague:
      return "Prague";
  }
  return "Unknown";
}

/// Check if fork supports EIP-1559 (base fee)
/// All supported forks (Shanghai+) have EIP-1559
[[nodiscard]] constexpr bool fork_has_eip1559([[maybe_unused]] evm::Fork fork) { return true; }

/// Check if fork supports EIP-4844 (blobs)
/// Cancun and later have blob transactions
[[nodiscard]] constexpr bool fork_has_eip4844(const evm::Fork fork) {
  return fork >= evm::Fork::Cancun;
}

/// Check if fork requires withdrawals (Shanghai+)
/// All supported forks require withdrawals
[[nodiscard]] constexpr bool fork_requires_withdrawals([[maybe_unused]] evm::Fork fork) {
  return true;
}

/// Check if fork requires parent beacon root (Cancun+)
[[nodiscard]] constexpr bool fork_requires_beacon_root(const evm::Fork fork) {
  return fork >= evm::Fork::Cancun;
}

/// Check if fork supports EIP-7702 (SetCode transactions)
/// Prague has EIP-7702
[[nodiscard]] constexpr bool fork_has_eip7702(const evm::Fork fork) {
  return fork >= evm::Fork::Prague;
}

}  // namespace div0::cli

#endif  // DIV0_CLI_T8N_FORK_PARSER_H
