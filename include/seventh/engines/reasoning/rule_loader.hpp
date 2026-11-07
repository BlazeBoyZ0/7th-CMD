#pragma once

#include "seventh/engines/reasoning/reasoning_types.hpp"

#include <string>
#include <vector>

namespace seventh::reasoning {

class RuleLoader {
public:
    [[nodiscard]] bool load_file(const std::string& path,
                                 std::vector<Rule>& rules,
                                 std::string& error) const;
};

} // namespace seventh::reasoning
