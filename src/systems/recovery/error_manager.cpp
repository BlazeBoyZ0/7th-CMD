#include "seventh/systems/recovery/error_manager.hpp"

#include <utility>

namespace seventh::recovery {

void ErrorManager::record(ErrorRecord error) {
    if (error.message.empty()) return;
    errors_.push_back(std::move(error));
}

std::optional<ErrorRecord> ErrorManager::latest() const {
    if (errors_.empty()) return std::nullopt;
    return errors_.back();
}

const std::vector<ErrorRecord>& ErrorManager::history() const noexcept {
    return errors_;
}

void ErrorManager::clear() {
    errors_.clear();
}

} // namespace seventh::recovery
