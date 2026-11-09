#pragma once

#include "seventh/systems/verification/verification_types.hpp"

namespace seventh::verification {

class ResultVerifier {
public:
    [[nodiscard]] VerificationResult verify(
        const VerificationRequest& request) const;
};

} // namespace seventh::verification
