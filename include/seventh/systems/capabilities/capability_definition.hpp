#pragma once

#include <string>
#include <vector>

namespace seventh::capabilities {

struct CapabilityDefinition {
    std::string id;
    std::string description;
    std::vector<std::string> operations;
    std::vector<std::string> allowed_roots;
    bool enabled{false};
    bool read_only{true};
    bool requires_confirmation{false};
};

} // namespace seventh::capabilities
