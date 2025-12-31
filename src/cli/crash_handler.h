// crash_handler.h - Signal handler for stack traces and core dumps

#ifndef DIV0_CLI_CRASH_HANDLER_H
#define DIV0_CLI_CRASH_HANDLER_H

// Install crash handler for SIGSEGV, SIGABRT, SIGFPE, SIGBUS, SIGILL.
// Must be called at program startup before any potentially crashing code.
void install_crash_handler(void);

#endif // DIV0_CLI_CRASH_HANDLER_H
