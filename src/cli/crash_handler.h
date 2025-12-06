// crash_handler.h - Signal handler for stack traces and core dumps

#ifndef DIV0_CLI_CRASH_HANDLER_H
#define DIV0_CLI_CRASH_HANDLER_H

namespace div0::cli {

/**
 * @brief Install signal handlers for crash reporting.
 *
 * Installs handlers for SIGSEGV, SIGABRT, and SIGFPE that:
 * 1. Print a stack trace to stderr
 * 2. Re-raise the signal to generate a core dump
 *
 * For core dumps to be generated, ensure `ulimit -c unlimited` is set.
 */
void install_crash_handler();

}  // namespace div0::cli

#endif  // DIV0_CLI_CRASH_HANDLER_H
