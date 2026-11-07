#include "seventh/systems/information/data_parser.hpp"

#include <sstream>

namespace seventh::information {

bool DataParser::parse_key_value_lines(
    std::string_view input, DataRecord& record, std::string& error) const {
    std::istringstream stream{std::string(input)};
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(stream, line)) {
        ++line_number;
        if (line.empty()) continue;
        const auto separator = line.find('=');
        if (separator == std::string::npos || separator == 0) {
            error = "Expected key=value on line " + std::to_string(line_number) + ".";
            return false;
        }
        record[line.substr(0, separator)] = line.substr(separator + 1);
    }
    return true;
}

} // namespace seventh::information
