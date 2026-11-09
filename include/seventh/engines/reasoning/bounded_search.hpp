#pragma once

#include "seventh/engines/reasoning/reasoning_types.hpp"

#include <cstddef>
#include <vector>

namespace seventh::reasoning {

class BoundedSearch {
public:
    [[nodiscard]] BoundedSearchResult collect_relevant(
        const FactQuery& query, const std::vector<Fact>& facts,
        std::size_t max_candidates, std::size_t max_facts_examined) const;
};

} // namespace seventh::reasoning
