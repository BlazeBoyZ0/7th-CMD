#pragma once

#include "seventh/engines/reasoning/reasoning_types.hpp"

#include <cstddef>
#include <vector>

namespace seventh::reasoning {

class BoundedSearch {
public:
    [[nodiscard]] std::vector<Fact> collect_relevant(
        const FactQuery& query, const std::vector<Fact>& facts,
        std::size_t max_candidates) const;
};

} // namespace seventh::reasoning
