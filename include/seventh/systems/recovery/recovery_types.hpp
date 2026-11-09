#pragma once

#include <map>
#include <string>

namespace seventh::recovery {

struct ErrorRecord {
    std::string code;
    std::string message;
    std::string subsystem;
    bool recoverable{false};
    std::map<std::string, std::string> context;
};

enum class RecoveryAction {
    Retry,
    RequestInformation,
    Abort,
    NoAction
};

struct RecoveryDecision {
    RecoveryAction action{RecoveryAction::NoAction};
    std::string explanation;
};

} // namespace seventh::recovery
