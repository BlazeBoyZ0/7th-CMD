#include "seventh/systems/verification/verification_system.hpp"

namespace seventh::verification {

VerificationResult VerificationSystem::verify(
    const VerificationRequest& request) const {
    return verifier_.verify(request);
}

} // namespace seventh::verification
