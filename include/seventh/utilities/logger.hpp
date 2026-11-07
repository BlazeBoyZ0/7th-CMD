#pragma once

#include <string_view>

namespace seventh::utilities {

enum class LogLevel {
    Error,
    Warning,
    Info,
    Debug
};

class Logger {
public:
    explicit Logger(LogLevel minimum_level = LogLevel::Info) noexcept;
    void set_level(LogLevel level) noexcept;
    void write(LogLevel level, std::string_view message) const;

private:
    LogLevel minimum_level_;
};

} // namespace seventh::utilities
