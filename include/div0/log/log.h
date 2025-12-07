#ifndef DIV0_LOG_LOG_H
#define DIV0_LOG_LOG_H

// Suppress warnings from spdlog headers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
// NOLINTBEGIN(misc-include-cleaner,clang-analyzer-optin.cplusplus.UninitializedObject)
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
// NOLINTEND(misc-include-cleaner,clang-analyzer-optin.cplusplus.UninitializedObject)

#include <string>
#include <string_view>

namespace div0::log {

// =============================================================================
// Configuration
// =============================================================================

/// Default log pattern: [time] [component] [level] message
inline constexpr const char* DEFAULT_PATTERN = "[%H:%M:%S.%e] [%^%l%$] [%n] %v";

/// Initialize the logging system with default settings.
/// Should be called once at program startup.
/// @param level The default minimum log level (default: info)
inline void init(spdlog::level::level_enum level = spdlog::level::info) {
  spdlog::set_level(level);
}

/// Set the global default log level for new loggers
inline void set_level(spdlog::level::level_enum level) { spdlog::set_level(level); }

/// Parse log level from string
inline spdlog::level::level_enum parse_level(std::string_view level_str) {
  if (level_str == "trace") {
    return spdlog::level::trace;
  }
  if (level_str == "debug") {
    return spdlog::level::debug;
  }
  if (level_str == "info") {
    return spdlog::level::info;
  }
  if (level_str == "warn" || level_str == "warning") {
    return spdlog::level::warn;
  }
  if (level_str == "error" || level_str == "err") {
    return spdlog::level::err;
  }
  if (level_str == "critical" || level_str == "fatal") {
    return spdlog::level::critical;
  }
  if (level_str == "off") {
    return spdlog::level::off;
  }
  return spdlog::level::info;  // Default
}

/// Set level from string
inline void set_level(std::string_view level_str) { set_level(parse_level(level_str)); }

// =============================================================================
// Component Loggers
// =============================================================================

/// Get or create a logger for a named component.
/// Thread-safe. Returns the same logger instance for the same name.
inline spdlog::logger& get(const std::string& name) {
  auto logger = spdlog::get(name);
  if (!logger) {
    logger = spdlog::stderr_color_mt(name);
    logger->set_pattern(DEFAULT_PATTERN);
  }
  return *logger;
}

/// Set log level for a specific component
inline void set_component_level(const std::string& name, spdlog::level::level_enum level) {
  auto logger = spdlog::get(name);
  if (logger) {
    logger->set_level(level);
  }
}

/// Set log level for a specific component from string
inline void set_component_level(const std::string& name, std::string_view level_str) {
  set_component_level(name, parse_level(level_str));
}

// =============================================================================
// Predefined Component Loggers
// =============================================================================
// These provide convenient access to loggers for each major subsystem.
// Usage: div0::log::t8n().info("message");

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
inline spdlog::logger& t8n() { return get("t8n"); }
inline spdlog::logger& evm() { return get("evm"); }
inline spdlog::logger& state() { return get("state"); }
inline spdlog::logger& trie() { return get("trie"); }
inline spdlog::logger& rlp() { return get("rlp"); }
inline spdlog::logger& crypto() { return get("crypto"); }
inline spdlog::logger& db() { return get("db"); }
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace div0::log

#endif  // DIV0_LOG_LOG_H
