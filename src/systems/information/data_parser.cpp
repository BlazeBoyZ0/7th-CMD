#include "seventh/systems/information/data_parser.hpp"

#include <sstream>
#include <utility>

namespace seventh::information {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

} // namespace

bool DataParser::parse_key_value_lines(
    std::string_view input, DataRecord& record, std::string& error) const {
    std::istringstream stream{std::string(input)};
    DataRecord parsed;
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(stream, line)) {
        ++line_number;
        line = trim(std::move(line));
        if (line.empty() || line.starts_with('#')) continue;

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            error = "Expected key=value on line " + std::to_string(line_number) + ".";
            return false;
        }

        const auto key = trim(line.substr(0, separator));
        const auto value = trim(line.substr(separator + 1));
        if (key.empty()) {
            error = "The key is empty on line " + std::to_string(line_number) + ".";
            return false;
        }
        parsed[key] = value;
    }

    record = std::move(parsed);
    error.clear();
    return true;
}

} // namespace seventh::information
