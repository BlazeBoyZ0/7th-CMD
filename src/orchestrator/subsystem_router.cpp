#include "seventh/orchestrator/subsystem_router.hpp"

#include "seventh/utilities/json_loader.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

namespace seventh {
namespace {

const utilities::JsonValue* member(
    const utilities::JsonValue::Object& object, const std::string& key) {
    const auto found = object.find(key);
    return found == object.end() ? nullptr : &found->second;
}

bool read_string_array(const utilities::JsonValue& value,
                       StringList& output, const std::string& field,
                       std::string& error, bool allow_empty) {
    const auto* array = value.as_array();
    if (!array || (!allow_empty && array->empty())) {
        error = "Subsystem profile field '" + field + "' must be an array" +
                (allow_empty ? "." : " with at least one entry.");
        return false;
    }

    StringList parsed;
    std::set<std::string> unique;
    for (const auto& entry : *array) {
        const auto* text = entry.as_string();
        if (!text || text->empty() || !unique.insert(*text).second) {
            error = "Subsystem profile field '" + field +
                    "' must contain unique, non-empty strings.";
            return false;
        }
        parsed.push_back(*text);
    }
    output = std::move(parsed);
    return true;
}

} // namespace

bool SubsystemRouter::load_profiles(const std::string& path,
                                    std::string& error, bool merge) {
    utilities::JsonLoader loader;
    const auto parsed = loader.load_file(path, error);
    if (!parsed) return false;

    const auto* root = parsed->as_object();
    if (!root) {
        error = "The subsystem profile root must be a JSON object.";
        return false;
    }

    const auto* version_value = member(*root, "schema_version");
    const auto* version = version_value ? version_value->as_number() : nullptr;
    if (!version || *version != 1.0) {
        error = "The subsystem profile schema version is unsupported.";
        return false;
    }

    const auto* profiles_value = member(*root, "subsystems");
    const auto* profiles = profiles_value ? profiles_value->as_array() : nullptr;
    if (!profiles) {
        error = "The subsystem profile file must contain a subsystems array.";
        return false;
    }

    auto updated = merge ? profiles_ : std::map<std::string, SubsystemProfile>{};
    std::set<std::string> identifiers;

    for (std::size_t index = 0; index < profiles->size(); ++index) {
        const auto* object = (*profiles)[index].as_object();
        if (!object) {
            error = "Subsystem profile " + std::to_string(index) +
                    " must be a JSON object.";
            return false;
        }

        const auto* id_value = member(*object, "id");
        const auto* id = id_value ? id_value->as_string() : nullptr;
        if (!id || id->empty() || !identifiers.insert(*id).second) {
            error = "Subsystem profile " + std::to_string(index) +
                    " has a missing, invalid, or duplicate ID.";
            return false;
        }

        const auto existing = updated.find(*id);
        SubsystemProfile profile;
        if (existing != updated.end()) profile = existing->second;
        profile.id = *id;

        if (const auto* handles_value = member(*object, "handles")) {
            if (!read_string_array(*handles_value, profile.handles,
                                   "handles", error, false)) {
                return false;
            }
        } else if (existing == updated.end()) {
            error = "A new subsystem profile requires a handles array.";
            return false;
        }

        if (const auto* requirements_value = member(*object, "requires")) {
            if (!read_string_array(*requirements_value, profile.requirements,
                                   "requires", error, true)) {
                return false;
            }
        }

        if (const auto* enabled_value = member(*object, "enabled")) {
            const auto* enabled = enabled_value->as_bool();
            if (!enabled) {
                error = "Subsystem profile enabled must be a boolean.";
                return false;
            }
            profile.enabled = *enabled;
        }

        if (const auto* priority_value = member(*object, "priority")) {
            const auto* priority = priority_value->as_number();
            if (!priority || !std::isfinite(*priority) ||
                std::floor(*priority) != *priority ||
                *priority < -1000.0 || *priority > 1000.0) {
                error = "Subsystem profile priority must be an integer from -1000 to 1000.";
                return false;
            }
            profile.priority = static_cast<int>(*priority);
        }

        updated[*id] = std::move(profile);
    }

    profiles_ = std::move(updated);
    error.clear();
    return true;
}

std::optional<SubsystemProfile> SubsystemRouter::profile_for(
    const std::string& subsystem_id) const {
    const auto found = profiles_.find(subsystem_id);
    if (found == profiles_.end()) return std::nullopt;
    return found->second;
}

std::shared_ptr<Subsystem> SubsystemRouter::route(
    const std::vector<std::shared_ptr<Subsystem>>& subsystems,
    const SubsystemRequest& request) const {
    std::shared_ptr<Subsystem> selected;
    int selected_priority = -1001;

    for (const auto& subsystem : subsystems) {
        if (!subsystem || !subsystem->can_handle(request)) continue;

        const auto profile = profiles_.find(std::string(subsystem->id()));
        int priority = 0;
        if (profile != profiles_.end()) {
            if (!profile->second.enabled) continue;
            const auto& handles = profile->second.handles;
            if (std::find(handles.begin(), handles.end(), request.operation) ==
                handles.end()) {
                continue;
            }
            priority = profile->second.priority;

            bool requirements_met = true;
            for (const auto& requirement : profile->second.requirements) {
                if (std::find(request.constraints.begin(), request.constraints.end(),
                              requirement) == request.constraints.end()) {
                    requirements_met = false;
                    break;
                }
            }
            if (!requirements_met) continue;
        }

        if (!selected || priority > selected_priority) {
            selected = subsystem;
            selected_priority = priority;
        }
    }

    return selected;
}

} // namespace seventh
