// exit_codes.h - Standard exit codes for CLI commands

#ifndef DIV0_CLI_EXIT_CODES_H
#define DIV0_CLI_EXIT_CODES_H

// Exit codes following conventions from Ethereum execution clients.
// Prefixed with DIV0_ to avoid conflicts with system EXIT_SUCCESS/EXIT_FAILURE.
typedef enum {
  DIV0_EXIT_SUCCESS = 0,
  DIV0_EXIT_GENERAL_ERROR = 1,
  DIV0_EXIT_EVM_ERROR = 2,
  DIV0_EXIT_CONFIG_ERROR = 3,
  DIV0_EXIT_MISSING_BLOCK_HASH = 4,
  DIV0_EXIT_JSON_ERROR = 10,
  DIV0_EXIT_IO_ERROR = 11,
  DIV0_EXIT_RLP_ERROR = 12,
} div0_exit_code_t;

#endif // DIV0_CLI_EXIT_CODES_H
