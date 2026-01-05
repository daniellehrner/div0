// crash_handler.c - Signal handler for stack traces and core dumps
//
// NOTE: Signal handlers have strict constraints - they must only use
// async-signal-safe functions. This means:
// - No memory allocation (malloc, calloc, etc.)
// - Only specific POSIX functions (write, _exit, signal, raise, etc.)
// Therefore, this file uses hand-rolled integer-to-string conversion
// instead of snprintf (which is NOT async-signal-safe).

#include "crash_handler.h"

#include <backtrace.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

// Backtrace state (initialized at startup, read-only after that)
static struct backtrace_state *g_bt_state = nullptr;

// Write string to stderr (async-signal-safe)
static void write_stderr(const char *msg) {
  write(STDERR_FILENO, msg, strlen(msg));
}

// Write an unsigned integer to stderr (async-signal-safe)
static void write_stderr_uint(unsigned int val) {
  char buf[16];
  char *p = buf + sizeof(buf) - 1;
  *p = '\0';
  if (val == 0) {
    *--p = '0';
  } else {
    while (val > 0) {
      *--p = (char)('0' + (val % 10));
      val /= 10;
    }
  }
  write_stderr(p);
}

// Error callback for libbacktrace
static void error_callback(void *data, const char *msg, int errnum) {
  (void)data;
  (void)errnum;
  write_stderr("  backtrace error: ");
  write_stderr(msg);
  write_stderr("\n");
}

// Full callback for each stack frame (async-signal-safe)
static int full_callback(void *data, uintptr_t pc, const char *filename, int lineno,
                         const char *function) {
  (void)pc;
  int *frame_num = (int *)data;

  // Build output using only write() and string literals
  write_stderr("  #");
  write_stderr_uint((unsigned int)(*frame_num)++);
  write_stderr(" ");
  write_stderr(function != nullptr ? function : "??");
  write_stderr(" at ");
  write_stderr(filename != nullptr ? filename : "??");
  write_stderr(":");
  write_stderr_uint(lineno >= 0 ? (unsigned int)lineno : 0);
  write_stderr("\n");

  return 0; // Continue iterating
}

// Signal handler
static void crash_handler(int sig) {
  write_stderr("\n");
  write_stderr("========================================================================"
               "========\n");
  write_stderr("CRASH DETECTED\n");
  write_stderr("========================================================================"
               "========\n");
  write_stderr("Signal: ");

  // Map signal to name
  const char *sig_name;
  switch (sig) {
  case SIGSEGV:
    sig_name = "SIGSEGV (Segmentation fault)";
    break;
  case SIGABRT:
    sig_name = "SIGABRT (Aborted)";
    break;
  case SIGFPE:
    sig_name = "SIGFPE (Floating point exception)";
    break;
  case SIGBUS:
    sig_name = "SIGBUS (Bus error)";
    break;
  case SIGILL:
    sig_name = "SIGILL (Illegal instruction)";
    break;
  default:
    sig_name = "UNKNOWN";
    break;
  }
  write_stderr(sig_name);
  write_stderr("\n\n");

  write_stderr("Stack trace:\n");

  // Use libbacktrace for symbolized stack trace
  if (g_bt_state != nullptr) {
    int frame_num = 0;
    backtrace_full(g_bt_state, 1, full_callback, error_callback, &frame_num);
  } else {
    write_stderr("  (backtrace not available)\n");
  }

  write_stderr("\n");
  write_stderr("For core dump, ensure: ulimit -c unlimited\n");
  write_stderr("========================================================================"
               "========\n");

  // Restore default handler and re-raise to get core dump
  // NOLINTNEXTLINE(cert-err33-c) - in signal handler, can't handle errors
  signal(sig, SIG_DFL);
  // NOLINTNEXTLINE(cert-err33-c) - in signal handler, can't handle errors
  raise(sig);
}

void install_crash_handler(void) {
  // Initialize libbacktrace state
  // nullptr for filename means use /proc/self/exe (Linux) or executable path (macOS)
  g_bt_state = backtrace_create_state(nullptr, 0, error_callback, nullptr);

  // Use sigaction for more reliable signal handling
  struct sigaction sa;
  __builtin___memset_chk(&sa, 0, sizeof(sa), __builtin_object_size(&sa, 0));
  sa.sa_handler = crash_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = (int)SA_RESETHAND; // Reset to default after first signal

  if (sigaction(SIGSEGV, &sa, nullptr) != 0) {
    write_stderr("warning: failed to install crash handler for SIGSEGV\n");
  }
  if (sigaction(SIGABRT, &sa, nullptr) != 0) {
    write_stderr("warning: failed to install crash handler for SIGABRT\n");
  }
  if (sigaction(SIGFPE, &sa, nullptr) != 0) {
    write_stderr("warning: failed to install crash handler for SIGFPE\n");
  }
  if (sigaction(SIGBUS, &sa, nullptr) != 0) {
    write_stderr("warning: failed to install crash handler for SIGBUS\n");
  }
  if (sigaction(SIGILL, &sa, nullptr) != 0) {
    write_stderr("warning: failed to install crash handler for SIGILL\n");
  }
}
