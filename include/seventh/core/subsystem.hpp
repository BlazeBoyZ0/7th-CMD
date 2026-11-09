#pragma once

#include "seventh/core/subsystem_request.hpp"
#include "seventh/core/subsystem_result.hpp"

#include <string_view>

namespace seventh {

class Subsystem {
public:
    virtual ~Subsystem() = default;

    [[nodiscard]] virtual std::string_view id() const noexcept = 0;
    [[nodiscard]] virtual bool can_handle(const SubsystemRequest& request) const = 0;
    [[nodiscard]] virtual SubsystemResult handle(const SubsystemRequest& request) = 0;
};

} // namespace seventh
