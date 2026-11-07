#pragma once

#include "seventh/systems/recovery/recovery_types.hpp"

#include <cstddef>

namespace seventh::recovery {

class RecoveryManager {
public:
    [[nodiscard]] RecoveryDecision decide(
        const ErrorRecord& error, std::size_t attempts,
        std::size_t max_attempts) const;
};

} // namespace seventh::recovery
