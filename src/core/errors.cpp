#include "seventh/core/errors.hpp"

namespace seventh {

std::string_view to_string(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::None: return "none";
    case ErrorCode::InvalidInput: return "invalid_input";
    case ErrorCode::NotFound: return "not_found";
    case ErrorCode::PermissionDenied: return "permission_denied";
    case ErrorCode::UnsupportedOperation: return "unsupported_operation";
    case ErrorCode::ResourceUnavailable: return "resource_unavailable";
    case ErrorCode::IoFailure: return "io_failure";
    case ErrorCode::ParseFailure: return "parse_failure";
    case ErrorCode::InternalFailure: return "internal_failure";
    }
    return "unknown";
}

} // namespace seventh
