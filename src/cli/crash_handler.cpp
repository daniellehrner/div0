// crash_handler.cpp - Signal handler for stack traces and core dumps
//
// NOTE: Signal handlers have strict constraints - they must only use
// async-signal-safe functions. This means:
// - No C++ standard library (std::string, std::cout, etc.)
// - No memory allocation (new, malloc)
// - Only specific POSIX functions (write, _exit, signal, etc.)
// Therefore, this file uses C-style arrays and snprintf intentionally.

#include "crash_handler.h"

#include <backtrace.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstring>

namespace div0::cli {

namespace {

// Backtrace state (initialized at startup, read-only after that)
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
backtrace_state* g_bt_state = nullptr;

// Write string to stderr (async-signal-safe)
void write_stderr(const char* msg) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  write(STDERR_FILENO, msg, strlen(msg));
}

// Error callback for libbacktrace
void error_callback(void* /*data*/, const char* msg, int /*errnum*/) {
  write_stderr("  backtrace error: ");
  write_stderr(msg);
  write_stderr("\n");
}

// Full callback for each stack frame
int full_callback(void* data, uintptr_t /*pc*/, const char* filename, int lineno,
                  const char* function) {
  auto* frame_num = static_cast<int*>(data);

  // C-style array and snprintf required for async-signal-safety
  // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-bounds-array-to-pointer-decay,modernize-avoid-c-arrays)
  char buf[1024];
  snprintf(buf, sizeof(buf), "  #%d %s at %s:%d\n", (*frame_num)++,
           function != nullptr ? function : "??", filename != nullptr ? filename : "??", lineno);
  write_stderr(buf);
  // NOLINTEND(cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-bounds-array-to-pointer-decay,modernize-avoid-c-arrays)

  return 0;  // Continue iterating
}

// Signal handler
void crash_handler(int sig) {
  write_stderr("\n");
  write_stderr(
      "================================================================================\n");
  write_stderr("CRASH DETECTED\n");
  write_stderr(
      "================================================================================\n");
  write_stderr("Signal: ");

  // Map signal to name
  const char* sig_name = nullptr;
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
  write_stderr(
      "================================================================================\n");

  // Restore default handler and re-raise to get core dump
  signal(sig, SIG_DFL);
  raise(sig);
}

}  // namespace

void install_crash_handler() {
  // Initialize libbacktrace state
  // nullptr for filename means use /proc/self/exe
  g_bt_state = backtrace_create_state(nullptr, 0, error_callback, nullptr);

  signal(SIGSEGV, crash_handler);
  signal(SIGABRT, crash_handler);
  signal(SIGFPE, crash_handler);
  signal(SIGBUS, crash_handler);
  signal(SIGILL, crash_handler);
}

}  // namespace div0::cli
