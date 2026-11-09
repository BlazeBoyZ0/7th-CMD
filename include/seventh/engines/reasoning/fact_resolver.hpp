#pragma once

#include "seventh/engines/reasoning/reasoning_types.hpp"

#include <vector>

namespace seventh::reasoning {

struct FactResolution {
    TruthValue truth{TruthValue::Unknown};
    std::vector<Fact> matches;
    std::vector<Fact> supporting;
    std::vector<Fact> opposing;
    bool contradiction{false};
};

class FactResolver {
public:
    [[nodiscard]] FactResolution resolve(const FactQuery& query,
                                         const std::vector<Fact>& facts) const;
};

} // namespace seventh::reasoning
