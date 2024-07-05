#ifndef LOGGING_H
#define LOGGING_H

#include <chrono>
#include <print>
#include <source_location>
#include <stacktrace>
#include <utility>

namespace chove::logging::_impl {

enum class Severity : uint8_t { kInfo, kWarn, kError, kFatal };

inline std::string_view GetNameFromSeverity(const Severity severity) {
  switch (severity) {
    case Severity::kInfo:
      return "\033[1;34mINFO\033[0m";
    case Severity::kWarn:
      return "\033[1;33mWARN\033[0m";
    case Severity::kError:
      return "\033[1;31mERROR\033[0m";
    case Severity::kFatal:
      return "\033[1;4;31mFATAL\033[0m";
  }
  return "UNKNOWN";
}

template<class... Types>
void log_impl(
    const Severity severity,
    const std::source_location location,
    const std::format_string<Types...> fmt,
    Types &&...args
) {
  const std::string_view type = GetNameFromSeverity(severity);
  std::print("[{} {} {}:{}]: ", type, std::chrono::system_clock::now(), location.file_name(), location.line());
  std::println(fmt, std::forward<Types>(args)...);
  if (severity == Severity::kError || severity == Severity::kFatal) {
    std::println("Stacktrace:");
    std::println("{}", std::stacktrace::current());
  }
  if (severity == Severity::kFatal) {
    std::exit(1);
  }
}

inline void log_impl(const Severity severity, const std::source_location &location, const std::string &str) {
  const std::string_view type = GetNameFromSeverity(severity);
  std::print("[{} {} {}:{}]: ", type, std::chrono::system_clock::now(), location.file_name(), location.line());
  std::println("{}", str);
  if (severity == Severity::kError || severity == Severity::kFatal) {
    std::println("Stacktrace:");
    std::println("{}", std::stacktrace::current());
  }
  if (severity == Severity::kFatal) {
    std::exit(1);
  }
}
}  // namespace chove::logging::_impl

#define log_info(...) \
  chove::logging::_impl::log_impl(chove::logging::_impl::Severity::kInfo, std::source_location::current(), __VA_ARGS__)
#define log_warn(...) \
  chove::logging::_impl::log_impl(chove::logging::_impl::Severity::kWarn, std::source_location::current(), __VA_ARGS__)
#define log_error(...) \
  chove::logging::_impl::log_impl(chove::logging::_impl::Severity::kError, std::source_location::current(), __VA_ARGS__)
#define log_fatal(...) \
  chove::logging::_impl::log_impl(chove::logging::_impl::Severity::kFatal, std::source_location::current(), __VA_ARGS__)

#define log_assert(condition, ...) \
  if (!(condition)) {              \
    log_fatal(__VA_ARGS__);        \
  }

#endif  // LOGGING_H
