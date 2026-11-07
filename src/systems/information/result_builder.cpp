#include "seventh/systems/information/result_builder.hpp"

namespace seventh::information {

void ResultBuilder::build_summary(DataResult& result,
                                  const std::string& operation) const {
    if (!result.success) return;
    if (!result.scalar.empty()) {
        result.summary = operation + ": " + result.scalar;
    } else {
        result.summary = operation + " returned " +
            std::to_string(result.records.size()) + " record(s).";
    }
}

} // namespace seventh::information
