#pragma once

#include "seventh/systems/response/response_formatter.hpp"

namespace seventh::response {

class ResponseGenerator {
public:
    [[nodiscard]] std::string generate(const ResponseData& data) const;

private:
    ResponseFormatter formatter_;
};

} // namespace seventh::response
