#pragma once

#include "seventh/systems/response/response_types.hpp"

namespace seventh::response {

class ResponseFormatter {
public:
    [[nodiscard]] std::string format(const ResponseData& data) const;
};

} // namespace seventh::response
