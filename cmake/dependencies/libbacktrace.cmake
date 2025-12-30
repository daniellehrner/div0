# libbacktrace.cmake - Stack trace library with DWARF support
#
# libbacktrace is a library to produce symbolic backtraces.
# It uses DWARF debug info and provides async-signal-safe stack traces.
#
# Supports: Linux (ELF) and macOS (Mach-O)
# Source: https://github.com/ianlancetaylor/libbacktrace

FetchContent_Declare(
  libbacktrace
  GIT_REPOSITORY https://github.com/ianlancetaylor/libbacktrace.git
  GIT_TAG        b9e40069c0b47a722286b94eb5231f7f05c08713
  EXCLUDE_FROM_ALL
)

# Fetch the content (populates libbacktrace_SOURCE_DIR)
FetchContent_MakeAvailable(libbacktrace)

# libbacktrace uses autotools, but we can build it directly with CMake
# by compiling the required source files

# Common sources for all platforms
set(LIBBACKTRACE_SOURCES
  ${libbacktrace_SOURCE_DIR}/backtrace.c
  ${libbacktrace_SOURCE_DIR}/simple.c
  ${libbacktrace_SOURCE_DIR}/dwarf.c
  ${libbacktrace_SOURCE_DIR}/mmapio.c
  ${libbacktrace_SOURCE_DIR}/mmap.c
  ${libbacktrace_SOURCE_DIR}/posix.c
  ${libbacktrace_SOURCE_DIR}/fileline.c
  ${libbacktrace_SOURCE_DIR}/state.c
  ${libbacktrace_SOURCE_DIR}/sort.c
)

# Platform-specific sources
if(APPLE)
  # macOS uses Mach-O binary format
  list(APPEND LIBBACKTRACE_SOURCES ${libbacktrace_SOURCE_DIR}/macho.c)
else()
  # Linux (and other Unix-likes) use ELF binary format
  list(APPEND LIBBACKTRACE_SOURCES ${libbacktrace_SOURCE_DIR}/elf.c)
endif()

add_library(libbacktrace STATIC ${LIBBACKTRACE_SOURCES})

# Generate config.h for libbacktrace
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbacktrace-config)

if(APPLE)
  # macOS configuration
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libbacktrace-config/config.h
"/* Generated config.h for libbacktrace (macOS) */
#define HAVE_FCNTL 1
#define HAVE_MMAP 1
#define HAVE_DECL_STRNLEN 1
#define HAVE_STDINT_H 1
#define HAVE_GETIPINFO 1
#define HAVE_MACH_O_DYLD_H 1
")
else()
  # Linux configuration
  # Detect ELF size based on pointer size (32-bit vs 64-bit)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BACKTRACE_ELF_SIZE 64)
  else()
    set(BACKTRACE_ELF_SIZE 32)
  endif()

  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libbacktrace-config/config.h
"/* Generated config.h for libbacktrace (Linux) */
#define HAVE_DL_ITERATE_PHDR 1
#define HAVE_FCNTL 1
#define HAVE_LINK_H 1
#define HAVE_MMAP 1
#define HAVE_DECL_STRNLEN 1
#define HAVE_STDINT_H 1
#define HAVE_GETIPINFO 1
#define BACKTRACE_ELF_SIZE ${BACKTRACE_ELF_SIZE}
")
endif()

# Generate backtrace-supported.h
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libbacktrace-config/backtrace-supported.h
"/* Generated backtrace-supported.h */
#define BACKTRACE_SUPPORTED 1
#define BACKTRACE_USES_MALLOC 0
#define BACKTRACE_SUPPORTS_THREADS 1
#define BACKTRACE_SUPPORTS_DATA 1
")

target_include_directories(libbacktrace
  PUBLIC
    ${libbacktrace_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/libbacktrace-config
)

# Disable warnings for third-party code
target_compile_options(libbacktrace PRIVATE -w)

# Required for DWARF parsing
target_compile_definitions(libbacktrace PRIVATE
  HAVE_CONFIG_H
  $<$<NOT:$<PLATFORM_ID:Darwin>>:_GNU_SOURCE>
)

message(STATUS "  - libbacktrace : Stack trace symbolization library (${CMAKE_SYSTEM_NAME})")
