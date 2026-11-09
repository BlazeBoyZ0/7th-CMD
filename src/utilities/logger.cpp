#include "seventh/utilities/logger.hpp"

#include <iostream>

namespace seventh::utilities {
namespace {

const char* label(LogLevel level) {
    switch (level) {
    case LogLevel::Error: return "ERROR";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Info: return "INFO";
    case LogLevel::Debug: return "DEBUG";
    }
    return "UNKNOWN";
}

} // namespace

Logger::Logger(LogLevel minimum_level) noexcept
    : minimum_level_(minimum_level) {}

void Logger::set_level(LogLevel level) noexcept {
    minimum_level_ = level;
}

void Logger::write(LogLevel level, std::string_view message) const {
    if (static_cast<int>(level) > static_cast<int>(minimum_level_)) return;
    std::clog << "[7thcmd][" << label(level) << "] " << message << '\n';
}

} // namespace seventh::utilities
