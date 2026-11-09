#include "seventh/systems/information/information_processor.hpp"

namespace seventh::information {

InformationProcessor::InformationProcessor(const ProviderRegistry& registry)
    : acquisition_(registry) {}

DataResult InformationProcessor::process(const DataRequest& request) const {
    std::vector<DataRecord> records;
    DataResult result;
    auto effective_request = request;

    if (!request.provider_id.empty()) {
        std::string error;
        if (!acquisition_.acquire(request, records, error)) {
            result.error = error;
            return result;
        }
    }

    // Preserve the document's original text before record normalization trims
    // field values for tabular operations.
    if (effective_request.text_input.empty() &&
        effective_request.provider_id == "document" && !records.empty()) {
        const auto text = records.front().find("text");
        if (text != records.front().end()) effective_request.text_input = text->second;
    }

    normalizer_.normalize(records);
    result = operations_.execute(effective_request, records);
    result_builder_.build_summary(result, effective_request.operation);
    return result;
}

} // namespace seventh::information
