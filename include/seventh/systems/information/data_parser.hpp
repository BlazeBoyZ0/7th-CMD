#pragma once

#include "seventh/systems/information/information_types.hpp"

#include <string_view>

namespace seventh::information {

class DataParser {
public:
    [[nodiscard]] bool parse_key_value_lines(
        std::string_view input, DataRecord& record, std::string& error) const;
};

} // namespace seventh::information
