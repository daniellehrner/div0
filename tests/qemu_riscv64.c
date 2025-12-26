// QEMU user-mode support for RISC-V 64-bit bare-metal tests
//
// Provides:
// - _start entry point
// - stdin/stdout/stderr for picolibc tiny stdio
// - _exit for program termination
//
// QEMU user-mode emulates Linux syscalls, enabling bare-metal code to run
// on a host system without actual RISC-V hardware.

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Portable stringification macros
#define DIV0_STRINGIFY_(x) #x
#define DIV0_STRINGIFY(x) DIV0_STRINGIFY_(x)

// C23 unreachable() fallback for older libc
#ifndef unreachable
#define unreachable() __builtin_unreachable()
#endif

// Linux syscall numbers for RISC-V
#define SYS_READ 63
#define SYS_WRITE 64
#define SYS_EXIT 93

// Main function (defined in test code)
extern int main(void);

// =============================================================================
// Syscall wrappers
// =============================================================================

static inline long syscall1(long n, long a0) {
  register long a7 __asm__("a7") = n;
  register long arg0 __asm__("a0") = a0;
  __asm__ volatile("ecall" : "+r"(arg0) : "r"(a7) : "memory");
  return arg0;
}

static long syscall3(long n, long a0, long a1, long a2) {
  register long a7 __asm__("a7") = n;
  register long arg0 __asm__("a0") = a0;
  register long arg1 __asm__("a1") = a1;
  register long arg2 __asm__("a2") = a2;
  __asm__ volatile("ecall" : "+r"(arg0) : "r"(a7), "r"(arg1), "r"(arg2) : "memory");
  return arg0;
}

// =============================================================================
// TLS (Thread Local Storage) support
// =============================================================================

// Static TLS block for single-threaded bare-metal.
// 64 bytes is plenty for errno (4 bytes) plus any future TLS variables.
__attribute__((aligned(16))) static char tls_block[64];

// Minimal TLS support for single-threaded bare-metal.
// picolibc's CMake build doesn't include TLS initialization code,
// so we provide our own simple implementation.
static inline void _init_tls(void) {
  memset(tls_block, 0, sizeof(tls_block));
}

static inline void _set_tls(void *tls) {
  // Set the thread pointer register (tp) on RISC-V
  __asm__ volatile("mv tp, %0" : : "r"(tls));
}

// =============================================================================
// Program entry and exit
// =============================================================================

// Global pointer symbol defined by linker
extern char __global_pointer$[];

void _start(void) {
  // Initialize global pointer - required for accessing global/static variables
  // The gp register must be set before any code that uses gp-relative addressing
  __asm__ volatile(".option push\n"
                   ".option norelax\n"
                   "la gp, __global_pointer$\n"
                   ".option pop\n");

  // Initialize TLS before anything else - required for errno access in malloc
  _init_tls();
  _set_tls(tls_block);

  int ret = main();
  syscall1(SYS_EXIT, ret);
  unreachable();
}

void _exit(int status) {
  syscall1(SYS_EXIT, status);
  unreachable();
}

// =============================================================================
// picolibc tiny stdio support
// =============================================================================

static int stdio_putc(char c, [[maybe_unused]] FILE *file) {
  return syscall3(SYS_WRITE, 1, (long)&c, 1) == 1 ? (unsigned char)c : EOF;
}

static int stdio_getc([[maybe_unused]] FILE *file) {
  char c;
  return syscall3(SYS_READ, 0, (long)&c, 1) == 1 ? (unsigned char)c : EOF;
}

static FILE __stdin = FDEV_SETUP_STREAM(NULL, stdio_getc, NULL, _FDEV_SETUP_READ);
static FILE __stdout = FDEV_SETUP_STREAM(stdio_putc, NULL, NULL, _FDEV_SETUP_WRITE);
static FILE __stderr = FDEV_SETUP_STREAM(stdio_putc, NULL, NULL, _FDEV_SETUP_WRITE);

FILE *const stdin = &__stdin;
FILE *const stdout = &__stdout;
FILE *const stderr = &__stderr;

// =============================================================================
// POSIX I/O (used by some libc functions)
// =============================================================================

ssize_t write(int fd, const void *buf, size_t count) {
  return syscall3(SYS_WRITE, fd, (long)buf, count);
}

ssize_t read(int fd, void *buf, size_t count) {
  return syscall3(SYS_READ, fd, (long)buf, count);
}

// =============================================================================
// Heap support for picolibc sbrk
// =============================================================================

// 8MB static heap for QEMU user-mode testing
// Sized for worst-case EVM memory usage with 60M gas (~5.4MB) plus overhead.
// picolibc expects __heap_start and __heap_end to be linker symbols (addresses),
// not pointer variables. The symbol names ARE the addresses.
#define HEAP_SIZE (8 * 1024 * 1024)
__attribute__((used, aligned(16))) char __heap_start[HEAP_SIZE];
__asm__(".globl __heap_end\n.set __heap_end, __heap_start + " DIV0_STRINGIFY(HEAP_SIZE));
