#include "seventh/core/identifiers.hpp"

#include <atomic>
#include <chrono>
#include <sstream>

namespace seventh {

std::string make_identifier(const char* prefix) {
    static std::atomic<unsigned long long> sequence{0};
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();

    std::ostringstream stream;
    stream << prefix << '-' << now << '-' << ++sequence;
    return stream.str();
}

} // namespace seventh
