// exit_codes.h - Standard exit codes for CLI commands

#ifndef DIV0_CLI_EXIT_CODES_H
#define DIV0_CLI_EXIT_CODES_H

#include <cstdint>

namespace div0::cli {

/**
 * @brief Exit codes for CLI commands.
 *
 * Following conventions from other Ethereum execution clients.
 */
enum class ExitCode : std::uint8_t {
  Success = 0,
  GeneralError = 1,
  EvmError = 2,
  ConfigError = 3,
  MissingBlockHash = 4,
  JsonError = 10,
  IoError = 11,
  RlpError = 12,
};

}  // namespace div0::cli

#endif  // DIV0_CLI_EXIT_CODES_H
