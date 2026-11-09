#include "seventh/engines/grammar/grammar_subsystem.hpp"

#include <cstddef>
#include <string>
#include <utility>

namespace seventh::grammar {

GrammarSubsystem::GrammarSubsystem(GrammarConfig config)
    : engine_(std::move(config)) {}

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
    result.result_data["schema_version"] = std::to_string(intent.schema_version);
    result.result_data["cir_id"] = intent.id;
    result.result_data["valid"] = intent.valid ? "true" : "false";
    result.result_data["operation"] = intent.operation;
    result.result_data["entity_type"] = intent.entity_type;
    result.result_data["name"] = intent.name;
    result.result_data["source"] = intent.source;
    result.result_data["destination"] = intent.destination;
    result.result_data["query"] = intent.query;
    result.result_data["raw_input"] = intent.raw_input;

    // SubsystemResult currently transports string fields. Keep relationship
    // records structured and deterministic using indexed, typed field names.
    result.result_data["relationship_count"] =
        std::to_string(intent.relationships.size());
    for (std::size_t index = 0; index < intent.relationships.size(); ++index) {
        const auto prefix = "relationship." + std::to_string(index) + ".";
        const auto& relationship = intent.relationships[index];
        result.result_data[prefix + "subject"] = relationship.subject;
        result.result_data[prefix + "predicate"] = relationship.predicate;
        result.result_data[prefix + "object"] = relationship.object;
        result.result_data[prefix + "subject_type"] = relationship.subject_type;
        result.result_data[prefix + "object_type"] = relationship.object_type;
        result.result_data[prefix + "object_is_pronoun"] =
            relationship.object_is_pronoun ? "true" : "false";
    }

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
