#include "seventh/engines/grammar/grammar_subsystem.hpp"

namespace seventh::grammar {

std::string_view GrammarSubsystem::id() const noexcept {
    return "grammar";
}

bool GrammarSubsystem::can_handle(const SubsystemRequest& request) const {
    return request.operation == "interpret" &&
           request.input_data.contains("raw_input");
}

SubsystemResult GrammarSubsystem::handle(const SubsystemRequest& request) {
    SubsystemResult result;
    result.request_id = request.request_id;

    const auto found = request.input_data.find("raw_input");
    if (found == request.input_data.end() || found->second.empty()) {
        result.status = SubsystemStatus::NeedsInformation;
        result.missing_information.emplace_back("instruction text");
        return result;
    }

    const auto intent = engine_.interpret(found->second);
    result.result_data["valid"] = intent.valid ? "true" : "false";
    result.result_data["operation"] = intent.operation;
    result.result_data["entity_type"] = intent.entity_type;
    result.result_data["name"] = intent.name;
    result.result_data["source"] = intent.source;
    result.result_data["destination"] = intent.destination;
    result.result_data["query"] = intent.query;
    result.result_data["raw_input"] = intent.raw_input;

    if (!intent.valid) {
        result.status = intent.missing_information.empty()
            ? SubsystemStatus::Failure
            : SubsystemStatus::NeedsInformation;
        result.missing_information = intent.missing_information;
        result.error = intent.error;
    } else {
        result.status = SubsystemStatus::Success;
    }
    return result;
}

} // namespace seventh::grammar
