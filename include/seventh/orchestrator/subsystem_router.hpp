#pragma once

#include "seventh/core/subsystem.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace seventh {

struct SubsystemProfile {
    std::string id;
    StringList handles;
    StringList requirements;
    int priority{0};
    bool enabled{true};
};

class SubsystemRouter {
public:
    bool load_profiles(const std::string& path, std::string& error,
                       bool merge = false);

    [[nodiscard]] std::optional<SubsystemProfile> profile_for(
        const std::string& subsystem_id) const;

    [[nodiscard]] std::shared_ptr<Subsystem> route(
        const std::vector<std::shared_ptr<Subsystem>>& subsystems,
        const SubsystemRequest& request) const;

private:
    std::map<std::string, SubsystemProfile> profiles_;
};

} // namespace seventh
