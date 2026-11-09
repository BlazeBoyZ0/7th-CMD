#pragma once

#include <map>
#include <string>

namespace seventh::verification {

struct VerificationRequest {
    std::string operation;
    std::map<std::string, std::string> expected;
    std::map<std::string, std::string> actual;
};

struct VerificationResult {
    bool verified{false};
    std::map<std::string, std::string> mismatches;
    std::string summary;
};

} // namespace seventh::verification
