#pragma once

#include "seventh/systems/recovery/recovery_types.hpp"

#include <optional>
#include <vector>

namespace seventh::recovery {

class ErrorManager {
public:
    void record(ErrorRecord error);
    [[nodiscard]] std::optional<ErrorRecord> latest() const;
    [[nodiscard]] const std::vector<ErrorRecord>& history() const noexcept;
    void clear();

private:
    std::vector<ErrorRecord> errors_;
};

} // namespace seventh::recovery
