#include "seventh/systems/recovery/recovery_manager.hpp"

namespace seventh::recovery {

RecoveryDecision RecoveryManager::decide(
    const ErrorRecord& error, std::size_t attempts,
    std::size_t max_attempts) const {
    if (error.code == "NEEDS_INFORMATION") {
        return {RecoveryAction::RequestInformation,
                "The task needs additional information before it can continue."};
    }
    if (error.recoverable && attempts < max_attempts) {
        return {RecoveryAction::Retry,
                "The error is marked recoverable and the retry budget remains."};
    }
    return {RecoveryAction::Abort,
            "The error cannot be safely retried or the retry budget is exhausted."};
}

} // namespace seventh::recovery
