#pragma once

#include "seventh/systems/verification/result_verifier.hpp"

namespace seventh::verification {

class VerificationSystem {
public:
    [[nodiscard]] VerificationResult verify(
        const VerificationRequest& request) const;
private:
    ResultVerifier verifier_;
};

} // namespace seventh::verification
