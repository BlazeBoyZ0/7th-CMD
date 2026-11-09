#pragma once

#include <string>
#include <string_view>

namespace seventh {

enum class ErrorCode {
    None,
    InvalidInput,
    NotFound,
    PermissionDenied,
    UnsupportedOperation,
    ResourceUnavailable,
    IoFailure,
    ParseFailure,
    InternalFailure
};

struct Error {
    ErrorCode code{ErrorCode::None};
    std::string message;

    [[nodiscard]] explicit operator bool() const noexcept {
        return code != ErrorCode::None;
    }
};

[[nodiscard]] std::string_view to_string(ErrorCode code) noexcept;

} // namespace seventh
