#include "seventh/systems/response/response_generator.hpp"

namespace seventh::response {

std::string ResponseGenerator::generate(const ResponseData& data) const {
    return formatter_.format(data);
}

} // namespace seventh::response
