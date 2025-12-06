# Core Dumps and Crash Debugging for div0

## Overview

Guide for debugging div0 crashes using the built-in crash handler and core dumps.

## Built-in Crash Handler

div0 includes a crash handler that automatically prints human-readable stack traces when a crash occurs. This is enabled by default and uses libbacktrace for DWARF-aware symbolization.

### Example Output

```
================================================================================
CRASH DETECTED
================================================================================
Signal: SIGSEGV (Segmentation fault)

Stack trace:
  #0 crash_handler at /home/user/div0/src/cli/crash_handler.cpp:45
  #1 main at /home/user/div0/src/cli/main.cpp:42
  #2 __libc_start_call_main at ../sysdeps/nptl/libc_start_call_main.h:58
  #3 __libc_start_main_impl at ../csu/libc-start.c:360

For core dump, ensure: ulimit -c unlimited
================================================================================
```

### Handled Signals

- `SIGSEGV` - Segmentation fault (null pointer, out-of-bounds access)
- `SIGABRT` - Abort (failed assertions, ASan/UBSan errors)
- `SIGFPE` - Floating point exception (division by zero)
- `SIGBUS` - Bus error (alignment issues)
- `SIGILL` - Illegal instruction

---

## Core Dumps

Core dumps provide a full memory snapshot at crash time, enabling deeper analysis with GDB/LLDB.

### Per-Shell vs Global Configuration

Core dumps can be enabled at two levels:

| Level | Scope | Persistence | Use Case |
|-------|-------|-------------|----------|
| Per-shell | Current terminal session only | Until shell closes | Development/debugging |
| Global | All users/processes | Permanent (survives reboot) | Production servers |

### Per-Shell (Temporary)

Enable core dumps for the current shell session only:

```bash
# Enable unlimited core dumps
ulimit -c unlimited

# Verify
ulimit -c
# Output: unlimited

# Run div0 - if it crashes, core dump will be generated
./build/debug/div0 ...
```

**Note**: This only affects the current shell and any processes it spawns. Opening a new terminal resets to default (usually 0).

### Global (Permanent)

#### Method 1: /etc/security/limits.conf

Edit `/etc/security/limits.conf` (requires root):

```bash
# Add these lines:
*  soft  core  unlimited
*  hard  core  unlimited

# Or for a specific user:
daniel  soft  core  unlimited
daniel  hard  core  unlimited
```

**Note**: Requires logout/login to take effect.

#### Method 2: systemd (if using systemd)

Create `/etc/systemd/system.conf.d/coredump.conf`:

```ini
[Manager]
DefaultLimitCORE=infinity
```

Then reload: `sudo systemctl daemon-reexec`

### Core Dump Location

#### Check Current Location

```bash
cat /proc/sys/kernel/core_pattern
```

#### Common Patterns

| System | Pattern | Location |
|--------|---------|----------|
| Ubuntu (Apport) | `\|/usr/share/apport/apport ...` | `/var/crash/` |
| Systemd | `\|/usr/lib/systemd/systemd-coredump ...` | `coredumpctl` |
| Simple | `core` | Current directory |
| Custom | `/tmp/core.%e.%p` | `/tmp/core.<name>.<pid>` |

#### Set Custom Location (requires root)

```bash
# Write to /tmp with executable name and PID
sudo sysctl -w kernel.core_pattern=/tmp/core.%e.%p

# Make permanent (add to /etc/sysctl.conf)
echo "kernel.core_pattern=/tmp/core.%e.%p" | sudo tee -a /etc/sysctl.conf
```

Pattern variables:
- `%e` - executable name
- `%p` - PID
- `%t` - timestamp
- `%h` - hostname

---

## Analyzing Core Dumps

### Using GDB

```bash
# Load core dump
gdb ./build/debug/div0 /tmp/core.div0.12345

# Inside GDB
(gdb) bt              # Backtrace
(gdb) bt full         # Backtrace with local variables
(gdb) frame 3         # Select frame #3
(gdb) info locals     # Show local variables
(gdb) info registers  # CPU registers at crash
(gdb) list            # Source code around crash
(gdb) print var       # Print variable value
```

### Using LLDB

```bash
lldb ./build/debug/div0 -c /tmp/core.div0.12345

(lldb) bt all         # Backtrace all threads
(lldb) frame select 3 # Select frame
(lldb) frame variable # Local variables
(lldb) register read  # CPU registers
```

### Using coredumpctl (systemd)

```bash
# List recent crashes
coredumpctl list

# Debug most recent div0 crash
coredumpctl debug div0

# Export core dump to file
coredumpctl dump div0 > core.dump

# Show info about crash
coredumpctl info div0
```

---

## Build Types for Debugging

All build types include debug symbols for crash debugging:

| Build Type | Debug Symbols | Optimization | Sanitizers | Use Case |
|------------|---------------|--------------|------------|----------|
| Debug | Yes | None | ASan+UBSan | Development |
| RelWithDebInfo | Yes | -O2 | None | Profiling |
| Release | Yes | -O3+LTO | None | Production |

```bash
# Debug build (default)
make debug

# Release build (with debug symbols)
make release
```

**Note**: Release builds include `-g` for debug symbols, enabling human-readable stack traces from the crash handler without impacting runtime performance.

---

## Quick Reference

```bash
# Enable core dumps (current shell)
ulimit -c unlimited

# Check core pattern
cat /proc/sys/kernel/core_pattern

# Set simple core pattern
sudo sysctl -w kernel.core_pattern=/tmp/core.%e.%p

# Debug with GDB
gdb ./build/debug/div0 /path/to/core

# Debug with coredumpctl
coredumpctl debug div0

# Full backtrace in GDB
(gdb) bt full
```

---

## Troubleshooting

### Core dump not generated

1. Check ulimit: `ulimit -c` (should be `unlimited` or a large number)
2. Check pattern: `cat /proc/sys/kernel/core_pattern`
3. Check disk space in target directory
4. Check if Apport is intercepting: look in `/var/crash/`

### Stack trace shows `??` instead of function names

1. Ensure debug build: `make debug`
2. Check `-rdynamic` is in linker flags (already configured in CMakeLists.txt)
3. Verify binary has symbols: `nm ./build/debug/div0 | head`

### Ubuntu/Apport intercepts cores

Disable Apport temporarily:

```bash
sudo systemctl stop apport
# Or
sudo sysctl -w kernel.core_pattern=/tmp/core.%e.%p
```