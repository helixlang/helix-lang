#ifndef __HELIX_LOGGER_H__
#define __HELIX_LOGGER_H__

#include <neo-pprint/include/ansi_colors.hh>
#include <neo-pprint/include/hxpprint.hh>

inline bool NO_LOGS = false;

enum class LogLevel { Debug, Info, Warning, Error };

namespace helix {
template <LogLevel l, typename... Args>
void log(Args &&...args) {
    std::string prefix;

    if (NO_LOGS) {
        return;
    }

    if constexpr (l == LogLevel::Debug) {
        prefix = std::string(colors::fg16::gray) + "debug: ";
    } else if constexpr (l == LogLevel::Info) {
        prefix = std::string(colors::fg16::green) + "info: ";
    } else if constexpr (l == LogLevel::Warning) {
        prefix = std::string(colors::fg16::yellow) + "warning: ";
    } else if constexpr (l == LogLevel::Error) {
        prefix = std::string(colors::fg16::red) + "error: ";
    }

    std::cout << std::flush;
    print("\r", prefix, std::string(colors::reset), std::forward<Args>(args)..., "\r");
}

template <LogLevel l, typename... Args>
void log_opt(bool enable, Args &&...args) {
    std::string prefix;

    if (!enable || NO_LOGS) {
        return;
    }

    if constexpr (l == LogLevel::Debug) {
        prefix = std::string(colors::fg16::gray) + "debug: ";
    } else if constexpr (l == LogLevel::Info) {
        prefix = std::string(colors::fg16::green) + "info: ";
    } else if constexpr (l == LogLevel::Warning) {
        prefix = std::string(colors::fg16::yellow) + "warning: ";
    } else if constexpr (l == LogLevel::Error) {
        prefix = std::string(colors::fg16::red) + "error: ";
    }

    std::cout << std::flush;
    print("\r", prefix, std::string(colors::reset), std::forward<Args>(args)..., "\r");
}
}  // namespace helix

#endif  // __HELIX_LOGGER_H__