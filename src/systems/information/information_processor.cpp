#include "seventh/systems/information/information_processor.hpp"

namespace seventh::information {

InformationProcessor::InformationProcessor(const ProviderRegistry& registry)
    : acquisition_(registry) {}

DataResult InformationProcessor::process(const DataRequest& request) const {
    std::vector<DataRecord> records;
    DataResult result;
    if (!request.provider_id.empty()) {
        std::string error;
        if (!acquisition_.acquire(request, records, error)) {
            result.error = error;
            return result;
        }
    }

    normalizer_.normalize(records);
    result = operations_.execute(request, records);
    result_builder_.build_summary(result, request.operation);
    return result;
}

} // namespace seventh::information
