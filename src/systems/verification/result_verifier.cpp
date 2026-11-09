#include "seventh/systems/verification/result_verifier.hpp"

namespace seventh::verification {

VerificationResult ResultVerifier::verify(
    const VerificationRequest& request) const {
    VerificationResult result;
    for (const auto& [key, expected_value] : request.expected) {
        const auto actual = request.actual.find(key);
        if (actual == request.actual.end()) {
            result.mismatches[key] = "<missing>";
        } else if (actual->second != expected_value) {
            result.mismatches[key] = actual->second;
        }
    }

    result.verified = result.mismatches.empty();
    result.summary = result.verified
        ? "The observed result matches all expected values."
        : "One or more expected values were missing or did not match.";
    return result;
}

} // namespace seventh::verification
