#include "seventh/orchestrator/command_subsystem.hpp"

#include "seventh/platform/linux/linux_operation_executor.hpp"
#include "seventh/systems/entity_resolution/entity_types.hpp"
#include "seventh/systems/permissions/permission_types.hpp"
#include "seventh/systems/verification/verification_types.hpp"
#include "seventh/systems/information/information_processor.hpp"
#include "seventh/systems/information/platform_data_providers.hpp"
#include "seventh/systems/information/provider_registry.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

namespace seventh {
namespace {

std::string lower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string entity_key(std::string value) {
    const auto first = value.find_first_not_of(" \\t\\r\\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \\t\\r\\n.,;:");
    if (last == std::string::npos || last < first) return {};
    return lower(value.substr(first, last - first + 1));
}

SubsystemResult failure(const RequestId& id, std::string message) {
    SubsystemResult result;
    result.request_id = id;
    result.status = SubsystemStatus::Failure;
    result.error = std::move(message);
    return result;
}

SubsystemResult needs_choice(
    const RequestId& id, const std::string& kind,
    const std::vector<entity_resolution::EntityCandidate>& candidates,
    bool invalid_answer = false) {
    SubsystemResult result;
    result.request_id = id;
    result.status = SubsystemStatus::NeedsInformation;
    result.missing_information.emplace_back(kind + " choice");
    if (invalid_answer) {
        result.error = "The supplied choice did not identify exactly one matching " + kind + ".";
    } else if (candidates.size() == 1) {
        result.error = "The best matching " + kind +
            " is uncertain. Confirm whether this is the intended target.";
    } else {
        result.error = "More than one matching " + kind + " was found.";
    }

    result.result_data["choice_kind"] = kind;
    result.result_data["choice_count"] = std::to_string(candidates.size());
    std::string options;
    constexpr std::size_t max_options = 10;
    const auto displayed = std::min(candidates.size(), max_options);
    for (std::size_t index = 0; index < displayed; ++index) {
        const auto& candidate = candidates[index];
        if (!options.empty()) options += '\n';
        options += std::to_string(index + 1) + ". " + candidate.name +
                   " | " + candidate.id;
        if (!candidate.location.empty()) options += " | " + candidate.location;
    }
    result.result_data["choice_options"] = std::move(options);
    result.result_data["choice_truncated"] =
        candidates.size() > displayed ? "true" : "false";
    return result;
}

std::optional<entity_resolution::EntityCandidate> choose_candidate(
    const SubsystemRequest& request, const std::string& field,
    const std::vector<entity_resolution::EntityCandidate>& candidates,
    bool& invalid_answer) {
    invalid_answer = false;
    const auto answer_entry = request.context.find("clarification." + field);
    if (answer_entry == request.context.end()) return std::nullopt;

    const auto answer = lower(answer_entry->second);
    if (answer.empty()) {
        invalid_answer = true;
        return std::nullopt;
    }

    std::vector<const entity_resolution::EntityCandidate*> exact_matches;
    for (const auto& candidate : candidates) {
        if (lower(candidate.id) == answer || lower(candidate.name) == answer ||
            lower(candidate.location) == answer) {
            exact_matches.push_back(&candidate);
        }
    }
    if (exact_matches.size() == 1) return *exact_matches.front();
    if (exact_matches.size() > 1) {
        invalid_answer = true;
        return std::nullopt;
    }

    std::size_t index = 0;
    const auto parsed = std::from_chars(
        answer.data(), answer.data() + answer.size(), index);
    if (parsed.ec == std::errc{} && parsed.ptr == answer.data() + answer.size() &&
        index > 0 && index <= candidates.size()) {
        return candidates[index - 1];
    }

    invalid_answer = true;
    return std::nullopt;
}
std::vector<std::string> split_instruction_chain(const std::string& input) {
    const auto lowered = lower(input);
    std::vector<std::string> clauses;
    std::size_t start = 0;
    while (start < input.size()) {
        const auto and_then = lowered.find(" and then ", start);
        const auto then = lowered.find(" then ", start);
        const auto than = lowered.find(" than ", start);
        std::size_t position = std::string::npos;
        std::size_t delimiter_size = 0;
        if (and_then != std::string::npos &&
            (then == std::string::npos || and_then <= then) &&
            (than == std::string::npos || and_then <= than)) {
            position = and_then;
            delimiter_size = 10;
        } else if (then != std::string::npos &&
                   (than == std::string::npos || then <= than)) {
            position = then;
            delimiter_size = 6;
        } else if (than != std::string::npos) {
            const auto next = than + 6;
            const auto next_space = lowered.find(' ', next);
            const auto action = lowered.substr(next,
                next_space == std::string::npos ? std::string::npos : next_space - next);
            static const std::set<std::string> actions{
                "create", "make", "rename", "move", "copy", "delete",
                "open", "launch", "search", "find", "count"
            };
            if (!actions.contains(action)) break;
            position = than;
            delimiter_size = 6;
        }
        if (position == std::string::npos) break;
        const auto clause = input.substr(start, position - start);
        if (!clause.empty()) clauses.push_back(clause);
        start = position + delimiter_size;
    }
    const auto final_clause = input.substr(start);
    if (!final_clause.empty()) clauses.push_back(final_clause);
    return clauses;
}

} // namespace

CommandSubsystem::CommandSubsystem(
    platform::PlatformInterface& platform, grammar::GrammarConfig grammar_config,
    knowledge::KnowledgeSystem* knowledge)
    : platform_(platform), grammar_engine_(std::move(grammar_config)),
      permission_manager_(capabilities_), knowledge_(knowledge) {
    capabilities::CapabilityDefinition create_folder;
    create_folder.id = "filesystem.create_directory";
    create_folder.description = "Create a directory within the user's home directory.";
    create_folder.operations = {"create_directory"};
    create_folder.enabled = true;
    create_folder.read_only = false;
    if (const char* home = std::getenv("HOME")) {
        create_folder.allowed_roots.emplace_back(home);
    }
    capabilities_.register_capability(std::move(create_folder));

    capabilities::CapabilityDefinition open_folder;
    open_folder.id = "filesystem.open_path";
    open_folder.description = "Open a folder inside the user's home directory.";
    open_folder.operations = {"open_path"};
    open_folder.enabled = true;
    open_folder.read_only = true;
    if (const char* home = std::getenv("HOME")) {
        open_folder.allowed_roots.emplace_back(home);
    }
    capabilities_.register_capability(std::move(open_folder));

    capabilities::CapabilityDefinition search_files;
    search_files.id = "filesystem.search";
    search_files.description = "Search one directory within the user's home directory.";
    search_files.operations = {"search_files"};
    search_files.enabled = true;
    search_files.read_only = true;
    if (const char* home = std::getenv("HOME")) {
        search_files.allowed_roots.emplace_back(home);
    }
    capabilities_.register_capability(std::move(search_files));

    capabilities::CapabilityDefinition rename_path;
    rename_path.id = "filesystem.rename";
    rename_path.description = "Rename or move a filesystem entry inside the user's home directory.";
    rename_path.operations = {"rename_path"};
    rename_path.enabled = true;
    rename_path.read_only = false;
    if (const char* home = std::getenv("HOME")) {
        rename_path.allowed_roots.emplace_back(home);
    }
    capabilities_.register_capability(std::move(rename_path));

    capabilities::CapabilityDefinition copy_file;
    copy_file.id = "filesystem.copy";
    copy_file.description = "Copy a file inside the user's home directory.";
    copy_file.operations = {"copy_file"};
    copy_file.enabled = true;
    copy_file.read_only = false;
    if (const char* home = std::getenv("HOME")) {
        copy_file.allowed_roots.emplace_back(home);
    }
    capabilities_.register_capability(std::move(copy_file));

    capabilities::CapabilityDefinition delete_path;
    delete_path.id = "filesystem.delete";
    delete_path.description = "Delete a file or empty directory inside the user's home directory.";
    delete_path.operations = {"delete_path"};
    delete_path.enabled = true;
    delete_path.read_only = false;
    delete_path.requires_confirmation = true;
    if (const char* home = std::getenv("HOME")) {
        delete_path.allowed_roots.emplace_back(home);
    }
    capabilities_.register_capability(std::move(delete_path));

    capabilities::CapabilityDefinition read_text;
    read_text.id = "filesystem.read_text";
    read_text.description = "Read small text files inside the user's home directory.";
    read_text.operations = {"read_text"};
    read_text.enabled = true;
    read_text.read_only = true;
    if (const char* home = std::getenv("HOME")) {
        read_text.allowed_roots.emplace_back(home);
    }
    capabilities_.register_capability(std::move(read_text));

    capabilities::CapabilityDefinition launch_application;
    launch_application.id = "application.launch";
    launch_application.description = "Launch an installed desktop application.";
    launch_application.operations = {"launch_application"};
    launch_application.enabled = true;
    launch_application.read_only = false;
    capabilities_.register_capability(std::move(launch_application));

    execution_engine_.register_executor(
        std::make_shared<platform::linux_backend::LinuxOperationExecutor>(platform_));
    information::register_platform_data_providers(information_registry_, platform_);
}

bool CommandSubsystem::load_capabilities(const std::string& path,
                                         std::string& error) {
    return capabilities_.load_from_file(path, error);
}

bool CommandSubsystem::load_reasoning_rules(const std::string& path,
                                            std::string& error,
                                            bool merge) {
    return reasoning_engine_.load_rules_from_file(path, error, merge);
}

std::string_view CommandSubsystem::id() const noexcept {
    return "command";
}

bool CommandSubsystem::can_handle(const SubsystemRequest& request) const {
    return request.operation == "command" &&
           request.input_data.contains("raw_input");
}

std::string CommandSubsystem::resolve_destination(
    const std::string& destination) const {
    const char* home = std::getenv("HOME");
    if (!home) return {};

    const std::filesystem::path requested(destination);
    if (!destination.empty() && requested.is_absolute()) {
        return requested.lexically_normal().string();
    }

    const auto normalized = lower(destination);
    std::string preference_key;
    if (normalized == "desktop") preference_key = "desktop_path";
    else if (normalized == "documents") preference_key = "documents_path";
    else if (normalized == "downloads") preference_key = "downloads_path";
    else if (normalized == "pictures") preference_key = "pictures_path";
    else if (normalized == "workspace") preference_key = "default_workspace";

    if (knowledge_ && !preference_key.empty()) {
        const auto preference = knowledge_->get(
            knowledge::KnowledgeScope::UserPreference, preference_key);
        if (preference && !preference->value.empty()) {
            std::filesystem::path preferred(preference->value);
            if (preferred.is_relative()) preferred = std::filesystem::path(home) / preferred;
            return preferred.lexically_normal().string();
        }
    }

    if (destination.empty()) {
        if (knowledge_) {
            const auto preference = knowledge_->get(
                knowledge::KnowledgeScope::UserPreference, "default_workspace");
            if (preference && !preference->value.empty()) {
                std::filesystem::path preferred(preference->value);
                if (preferred.is_relative()) preferred = std::filesystem::path(home) / preferred;
                return preferred.lexically_normal().string();
            }
        }
        return {};
    }

    if (normalized == "home") return std::filesystem::path(home).string();
    if (normalized == "desktop") {
        return (std::filesystem::path(home) / "Desktop").string();
    }
    const auto* session = context_manager_.find("session");
    if (session) {
        const auto key = "folder:" + lower(destination);
        if (session->values.contains("ambiguous:" + key)) return {};
        const auto remembered = session->values.find(key);
        if (remembered != session->values.end()) return remembered->second;
    }
    return (std::filesystem::path(home) / requested).lexically_normal().string();
}

void CommandSubsystem::remember_folder(
    const std::string& name, const std::string& path) {
    auto& session = context_manager_.create("session");
    const auto key = "folder:" + lower(name);
    const auto ambiguous_key = "ambiguous:" + key;
    session.values["last_entity.type"] = "folder";
    session.values["last_entity.name"] = name;
    session.values["last_entity.path"] = path;
    session.values.erase("last_entity.id");

    if (session.values.contains(ambiguous_key)) return;
    const auto previous = session.values.find(key);
    if (previous != session.values.end() && previous->second != path) {
        session.values.erase(previous);
        session.values[ambiguous_key] = "true";
        return;
    }
    session.values[key] = path;
}

SubsystemResult CommandSubsystem::create_folder(
    const SubsystemRequest& request, const grammar::CanonicalIntent& intent) {
    auto destination = intent.destination;
    if (destination.empty()) {
        const auto answer = request.context.find("clarification.destination folder");
        if (answer != request.context.end()) destination = answer->second;
    }
    const auto destination_path = resolve_destination(destination);
    if (destination_path.empty()) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = SubsystemStatus::NeedsInformation;
        result.missing_information.emplace_back("destination folder");
        result.error = "Specify a destination folder or configure a default workspace.";
        return result;
    }

    const std::filesystem::path folder_name(intent.name);
    if (folder_name.empty() || folder_name.is_absolute() ||
        folder_name.has_parent_path() || intent.name == "." || intent.name == "..") {
        return failure(request.request_id,
            "A folder name must be a single name, not a path.");
    }

    std::error_code fs_error;
    if (!std::filesystem::is_directory(destination_path, fs_error) || fs_error) {
        return failure(request.request_id,
            "The destination folder does not exist or cannot be accessed.");
    }

    entity_resolution::ResolutionRequest resolution_request;
    resolution_request.query = std::filesystem::path(destination_path).filename().string();
    resolution_request.expected_type = entity_resolution::EntityType::Folder;
    resolution_request.context_hint = std::filesystem::path(destination_path).parent_path().string();
    resolution_request.candidates.push_back({
        destination_path, resolution_request.query,
        entity_resolution::EntityType::Folder, resolution_request.context_hint,
        "linux.filesystem", true, 0
    });
    const auto resolved = entity_resolver_.resolve(std::move(resolution_request));
    if (resolved.status != entity_resolution::ResolutionStatus::Found) {
        return failure(request.request_id, "The destination folder could not be resolved uniquely.");
    }

    const auto target_path =
        (std::filesystem::path(resolved.selected.id) / folder_name).lexically_normal().string();

    reasoning::Fact existence_fact;
    existence_fact.subject = target_path;
    existence_fact.property = "exists";
    existence_fact.value = platform_.filesystem().exists(target_path) ? "true" : "false";
    existence_fact.source = "linux.filesystem";
    existence_fact.confidence = 100;
    existence_fact.fresh = true;

    reasoning::ReasoningRequest reasoning_request;
    reasoning_request.query = {target_path, "exists", "false", true};
    reasoning_request.facts.push_back(std::move(existence_fact));
    const auto reasoning_result = reasoning_engine_.evaluate(reasoning_request);
    if (reasoning_result.truth != TruthValue::True) {
        return failure(request.request_id,
            reasoning_result.truth == TruthValue::False
                ? "A folder with that name already exists."
                : "The system could not determine whether the target already exists.");
    }

    planning::PlanningRequest planning_request;
    planning_request.goal = "Create folder " + intent.name + " in " + destination_path;
    planning::TaskDraft draft;
    draft.id = "create-folder";
    draft.operation = "create_directory";
    draft.input_data["path"] = target_path;
    planning_request.tasks.push_back(std::move(draft));
    auto plan = planning_engine_.create_plan(planning_request);
    if (!plan.valid || plan.tasks.empty()) {
        return failure(request.request_id,
            plan.errors.empty() ? "Unable to create a valid execution plan."
                                : plan.errors.front());
    }

    permissions::PermissionRequest permission_request;
    permission_request.capability_id = "filesystem.create_directory";
    permission_request.operation = "create_directory";
    permission_request.resource_path = target_path;
    permission_request.modifies_state = true;
    const auto permission = permission_manager_.authorize(permission_request);
    if (permission.decision != permissions::PermissionDecision::Allowed) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = permission.decision ==
            permissions::PermissionDecision::RequiresConfirmation
                ? SubsystemStatus::NeedsPermission : SubsystemStatus::Failure;
        result.error = permission.reason;
        return result;
    }

    if (!plan_state_manager_.start(plan, plan.tasks.front().id)) {
        return failure(request.request_id, "The planned task could not enter the running state.");
    }

    execution::ExecutionRequest execution_request;
    execution_request.task_id = request.task_id;
    execution_request.cir_id = intent.id;
    execution_request.plan_id = plan.id;
    execution_request.plan_step_id = plan.tasks.front().id;
    execution_request.operation = plan.tasks.front().operation;
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    execution_request.authorized_root = permission.authorized_root;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                executed.error + " The plan state could not be marked failed.");
        }
        return failure(request.request_id, executed.error);
    }
    verification::VerificationRequest verification_request;
    verification_request.operation = "create_directory";
    verification_request.expected["exists"] = "true";
    verification_request.actual["exists"] =
        platform_.filesystem().exists(target_path) ? "true" : "false";
    const auto verification_result = verification_system_.verify(verification_request);
    (void)execution_engine_.set_verification(request.task_id,
        verification_result.verified ? execution::VerificationOutcome::Passed
                                     : execution::VerificationOutcome::Failed);
    if (!verification_result.verified) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                "The operation ran, verification failed, and the plan state could not be marked failed.");
        }
        return failure(request.request_id,
            "The operation ran, but verification could not confirm the new folder.");
    }

    if (!plan_state_manager_.complete(plan, plan.tasks.front().id)) {
        return failure(request.request_id,
            "Execution succeeded, but the plan state could not be marked complete.");
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "create";
    result.result_data["entity_type"] = "folder";
    result.result_data["name"] = intent.name;
    result.result_data["destination"] = destination_path;
    result.result_data["path"] = target_path;
    result.result_data["verified"] = "true";
    remember_folder(intent.name, target_path);
    return result;
}

SubsystemResult CommandSubsystem::open_target(
    const SubsystemRequest& request, const grammar::CanonicalIntent& intent) {
    std::optional<platform::ApplicationEntry> selected_app;
    std::optional<entity_resolution::EntityCandidate> selected_folder;
    const auto wanted = lower(intent.name);

    if (wanted == "it" || wanted == "that" || wanted == "that folder") {
        const auto* session = context_manager_.find("session");
        if (!session || !session->values.contains("last_entity.type")) {
            return failure(request.request_id,
                "There is no recent target in the current session to refer to.");
        }
        const auto type = session->values.at("last_entity.type");
        if (type == "folder" && session->values.contains("last_entity.path")) {
            const auto path = session->values.at("last_entity.path");
            std::error_code context_error;
            if (!platform_.filesystem().exists(path) ||
                !std::filesystem::is_directory(path, context_error) || context_error) {
                return failure(request.request_id,
                    "The previously referenced folder is no longer available.");
            }
            selected_folder = entity_resolution::EntityCandidate{
                path,
                session->values.contains("last_entity.name")
                    ? session->values.at("last_entity.name") : "folder",
                entity_resolution::EntityType::Folder,
                std::filesystem::path(path).parent_path().string(),
                "session.context", true, 100
            };
        } else if (type == "application" &&
                   session->values.contains("last_entity.id")) {
            selected_app = platform_.applications().find(
                session->values.at("last_entity.id"));
            if (!selected_app) {
                return failure(request.request_id,
                    "The previously referenced application is no longer available.");
            }
        } else {
            return failure(request.request_id,
                "The previous target cannot be opened by this command.");
        }
    } else if (wanted == "browser") {
        selected_app = platform_.applications().find("browser");
        if (!selected_app) selected_app = platform_.applications().find_default("browser");
        if (!selected_app) {
            return failure(request.request_id,
                "No default browser is configured on this system.");
        }

        entity_resolution::ResolutionRequest default_request;
        default_request.query = selected_app->name;
        default_request.expected_type = entity_resolution::EntityType::Application;
        default_request.candidates.push_back({
            selected_app->id, selected_app->name,
            entity_resolution::EntityType::Application,
            selected_app->desktop_file, "xdg.default", true, 0
        });
        const auto resolved = entity_resolver_.resolve(std::move(default_request));
        if (resolved.status != entity_resolution::ResolutionStatus::Found) {
            return failure(request.request_id,
                "The configured default browser could not be resolved.");
        }
    } else {
        const char* home = std::getenv("HOME");
        if (!home) return failure(request.request_id,
            "The user's home directory is unavailable.");

        const std::vector<std::filesystem::path> roots{
            home,
            std::filesystem::path(home) / "Desktop",
            std::filesystem::path(home) / "Documents",
            std::filesystem::path(home) / "Downloads"
        };
        entity_resolution::ResolutionRequest folder_request;
        folder_request.query = intent.name;
        folder_request.expected_type = entity_resolution::EntityType::Folder;
        if (const auto* session = context_manager_.find("session")) {
            const auto key = "folder:" + wanted;
            const auto remembered = session->values.find(key);
            if (remembered != session->values.end()) {
                std::error_code remembered_error;
                if (std::filesystem::is_directory(remembered->second, remembered_error) &&
                    !remembered_error) {
                    folder_request.candidates.push_back({
                        remembered->second, std::filesystem::path(remembered->second).filename().string(),
                        entity_resolution::EntityType::Folder,
                        std::filesystem::path(remembered->second).parent_path().string(),
                        "session.context", true, 0
                    });
                }
            }
        }

        for (const auto& root : roots) {
            std::string list_error;
            const auto entries = platform_.filesystem().list(root.string(), list_error);
            for (const auto& entry : entries) {
                if (!entry.directory || lower(entry.name) != wanted) continue;
                folder_request.candidates.push_back({
                    entry.path, entry.name, entity_resolution::EntityType::Folder,
                    root.string(), "linux.filesystem", true, 0
                });
            }
        }

        const auto folder_match = entity_resolver_.resolve(std::move(folder_request));
        if (folder_match.status == entity_resolution::ResolutionStatus::Ambiguous) {
            bool invalid_answer = false;
            selected_folder = choose_candidate(
                request, "folder choice", folder_match.candidates, invalid_answer);
            if (!selected_folder) {
                return needs_choice(request.request_id, "folder",
                                    folder_match.candidates, invalid_answer);
            }
        } else if (folder_match.status == entity_resolution::ResolutionStatus::Found) {
            selected_folder = folder_match.selected;
        } else {
            std::string app_error;
            const auto applications = platform_.applications().list(app_error);
            entity_resolution::ResolutionRequest app_request;
            app_request.query = intent.name;
            app_request.expected_type = entity_resolution::EntityType::Application;
            for (const auto& app : applications) {
                app_request.candidates.push_back({
                    app.id, app.name, entity_resolution::EntityType::Application,
                    app.desktop_file, "linux.desktop", true, 0
                });
            }
            const auto app_match = entity_resolver_.resolve(std::move(app_request));
            if (app_match.status == entity_resolution::ResolutionStatus::Ambiguous) {
                bool invalid_answer = false;
                const auto choice = choose_candidate(
                    request, "application choice", app_match.candidates, invalid_answer);
                if (!choice) {
                    return needs_choice(request.request_id, "application",
                                        app_match.candidates, invalid_answer);
                }
                selected_app = platform_.applications().find(choice->id);
            } else if (app_match.status == entity_resolution::ResolutionStatus::Found) {
                selected_app = platform_.applications().find(app_match.selected.id);
            }
        }
    }

    std::string operation;
    std::map<std::string, std::string> operation_data;
    std::string capability_id;
    std::string entity_type;
    std::string resolved_name;

    if (selected_folder) {
        operation = "open_path";
        operation_data["path"] = selected_folder->id;
        capability_id = "filesystem.open_path";
        entity_type = "folder";
        resolved_name = selected_folder->name;
    } else if (selected_app) {
        operation = "launch_application";
        operation_data["application_id"] = selected_app->id;
        capability_id = "application.launch";
        entity_type = "application";
        resolved_name = selected_app->name;
    } else {
        return failure(request.request_id,
            "No installed application or matching folder was found.");
    }

    planning::PlanningRequest planning_request;
    planning_request.goal = "Open " + entity_type + " " + resolved_name;
    planning::TaskDraft draft;
    draft.id = "open-target";
    draft.operation = operation;
    draft.input_data = operation_data;
    planning_request.tasks.push_back(std::move(draft));
    auto plan = planning_engine_.create_plan(planning_request);
    if (!plan.valid || plan.tasks.empty()) {
        return failure(request.request_id, "Unable to create a valid open plan.");
    }

    permissions::PermissionRequest permission_request;
    permission_request.capability_id = capability_id;
    permission_request.operation = operation;
    if (operation == "open_path") permission_request.resource_path = operation_data["path"];
    permission_request.modifies_state = operation == "launch_application";
    const auto permission = permission_manager_.authorize(permission_request);
    if (permission.decision != permissions::PermissionDecision::Allowed) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = permission.decision ==
            permissions::PermissionDecision::RequiresConfirmation
                ? SubsystemStatus::NeedsPermission : SubsystemStatus::Failure;
        result.error = permission.reason;
        return result;
    }

    if (!plan_state_manager_.start(plan, plan.tasks.front().id)) {
        return failure(request.request_id, "The open task could not enter the running state.");
    }

    execution::ExecutionRequest execution_request;
    execution_request.task_id = request.task_id;
    execution_request.cir_id = intent.id;
    execution_request.plan_id = plan.id;
    execution_request.plan_step_id = plan.tasks.front().id;
    execution_request.operation = operation;
    execution_request.input_data = operation_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                executed.error + " The plan state could not be marked failed.");
        }
        return failure(request.request_id, executed.error);
    }
    verification::VerificationRequest verification_request;
    verification_request.operation = operation;
    verification_request.expected["started"] = "true";
    verification_request.actual["started"] =
        executed.output_data.contains("started") ? executed.output_data.at("started") : "false";
    const auto verification_result = verification_system_.verify(verification_request);
    (void)execution_engine_.set_verification(request.task_id,
        verification_result.verified ? execution::VerificationOutcome::Passed
                                     : execution::VerificationOutcome::Failed);
    if (!verification_result.verified) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                "The open request could not be verified and the plan state could not be marked failed.");
        }
        return failure(request.request_id, "The open request could not be verified.");
    }

    if (!plan_state_manager_.complete(plan, plan.tasks.front().id)) {
        return failure(request.request_id,
            "Execution succeeded, but the plan state could not be marked complete.");
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "open";
    result.result_data["entity_type"] = entity_type;
    result.result_data["name"] = resolved_name;
    auto& session = context_manager_.create("session");
    session.values["last_entity.type"] = entity_type;
    session.values["last_entity.name"] = resolved_name;
    if (operation == "open_path") {
        result.result_data["path"] = operation_data["path"];
        session.values["last_entity.path"] = operation_data["path"];
        session.values.erase("last_entity.id");
    } else {
        session.values["last_entity.id"] = operation_data["application_id"];
        session.values.erase("last_entity.path");
    }
    return result;
}


SubsystemResult CommandSubsystem::search_items(
    const SubsystemRequest& request, const grammar::CanonicalIntent& intent) {
    const char* home = std::getenv("HOME");
    if (!home) return failure(request.request_id, "The user's home directory is unavailable.");

    const auto preferred_workspace = intent.destination.empty()
        ? resolve_destination("") : std::string{};
    const auto directory = intent.destination.empty()
        ? (preferred_workspace.empty() ? std::string(home) : preferred_workspace)
        : resolve_destination(intent.destination);
    if (directory.empty()) return failure(request.request_id, "The search directory could not be resolved.");

    std::error_code ec;
    if (!std::filesystem::is_directory(directory, ec) || ec) {
        return failure(request.request_id, "The search location does not exist or cannot be accessed.");
    }

    permissions::PermissionRequest permission_request;
    permission_request.capability_id = "filesystem.search";
    permission_request.operation = "search_files";
    permission_request.resource_path = directory;
    const auto permission = permission_manager_.authorize(permission_request);
    if (permission.decision != permissions::PermissionDecision::Allowed) {
        return failure(request.request_id, permission.reason);
    }

    planning::PlanningRequest planning_request;
    planning_request.goal = "Search " + intent.entity_type + " entries in " + directory;
    planning::TaskDraft draft;
    draft.id = "search-items";
    draft.operation = "search_files";
    draft.input_data["path"] = directory;
    draft.input_data["query"] = intent.query;
    draft.input_data["entity_type"] = intent.entity_type;
    planning_request.tasks.push_back(std::move(draft));
    auto plan = planning_engine_.create_plan(planning_request);
    if (!plan.valid || plan.tasks.empty()) {
        return failure(request.request_id, "Unable to create a valid search plan.");
    }
    if (!plan_state_manager_.start(plan, plan.tasks.front().id)) {
        return failure(request.request_id, "The search task could not enter the running state.");
    }

    execution::ExecutionRequest execution_request;
    execution_request.task_id = request.task_id;
    execution_request.cir_id = intent.id;
    execution_request.plan_id = plan.id;
    execution_request.plan_step_id = plan.tasks.front().id;
    execution_request.operation = plan.tasks.front().operation;
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    execution_request.authorized_root = permission.authorized_root;

    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        const bool marked_failed = plan_state_manager_.fail(plan, plan.tasks.front().id);
        auto result = failure(request.request_id, executed.error);
        if (!marked_failed) result.error += " The search plan state could not be marked failed.";
        result.result_data["execution_record_id"] = execution_request.task_id;
        return result;
    }

    verification::VerificationRequest verification_request;
    verification_request.operation = "search_files";
    verification_request.expected["exists"] = "true";
    verification_request.actual["exists"] =
        std::filesystem::is_directory(directory, ec) && !ec ? "true" : "false";
    const auto verification_result = verification_system_.verify(verification_request);
    (void)execution_engine_.set_verification(
        execution_request.task_id,
        verification_result.verified ? execution::VerificationOutcome::Passed
                                     : execution::VerificationOutcome::Failed);
    if (!verification_result.verified) {
        const bool marked_failed = plan_state_manager_.fail(plan, plan.tasks.front().id);
        auto result = failure(request.request_id, "The search result could not be verified.");
        if (!marked_failed) result.error += " The search plan state could not be marked failed.";
        result.result_data["execution_record_id"] = execution_request.task_id;
        return result;
    }

    if (!plan_state_manager_.complete(plan, plan.tasks.front().id)) {
        auto result = failure(request.request_id, "The search plan could not be marked complete.");
        result.result_data["execution_record_id"] = execution_request.task_id;
        return result;
    }

    const auto output = [&executed](const std::string& key) -> std::string {
        const auto found = executed.output_data.find(key);
        return found == executed.output_data.end() ? std::string{} : found->second;
    };

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "search";
    result.result_data["entity_type"] = intent.entity_type;
    result.result_data["directory"] = directory;
    result.result_data["query"] = intent.query;
    result.result_data["count"] = output("count");
    result.result_data["displayed"] = output("displayed");
    result.result_data["truncated"] = output("truncated");
    result.result_data["matches"] = output("matches");
    result.result_data["execution_record_id"] = execution_request.task_id;
    result.result_data["verified"] = "true";
    return result;
}


SubsystemResult CommandSubsystem::modify_path(
    const SubsystemRequest& request, const grammar::CanonicalIntent& intent) {
    const char* home = std::getenv("HOME");
    if (!home) return failure(request.request_id, "The user's home directory is unavailable.");

    // Clarification answers are stored on the pending request. Apply the
    // supplied destination before resolving or validating the operation.
    std::string destination_value = intent.destination;
    if (destination_value.empty()) {
        const auto answer = request.context.find("clarification.destination");
        if (answer != request.context.end()) destination_value = answer->second;
    }

    const std::vector<std::filesystem::path> roots{
        home,
        std::filesystem::path(home) / "Desktop",
        std::filesystem::path(home) / "Documents",
        std::filesystem::path(home) / "Downloads"
    };

    std::string source_query = intent.source;
    const auto source_reference = lower(source_query);
    if (source_reference == "it" || source_reference == "that" ||
        source_reference == "that file" || source_reference == "that folder" ||
        source_reference == "previous file" || source_reference == "previous folder") {
        if (const auto* session = context_manager_.find("session")) {
            const auto last_path = session->values.find("last_entity.path");
            const auto last_type = session->values.find("last_entity.type");
            if (last_path != session->values.end() && last_type != session->values.end()) {
                const bool compatible =
                    intent.entity_type == "item" ||
                    (intent.entity_type == "folder" && last_type->second == "folder") ||
                    (intent.entity_type == "file" && last_type->second == "file");
                if (compatible) source_query = last_path->second;
            }
        }
    }

    entity_resolution::ResolutionRequest source_request;
    source_request.query = source_query;
    if (intent.entity_type == "file") {
        source_request.expected_type = entity_resolution::EntityType::File;
    } else if (intent.entity_type == "folder") {
        source_request.expected_type = entity_resolution::EntityType::Folder;
    }

    const std::filesystem::path source_as_path(source_query);
    std::filesystem::path resolved_source;
    if (source_as_path.is_absolute()) {
        resolved_source = source_as_path;
    } else {
        const auto home_relative = std::filesystem::path(home) / source_as_path;
        if (platform_.filesystem().exists(home_relative.string())) {
            resolved_source = home_relative;
        }
    }

    if (!resolved_source.empty() && platform_.filesystem().exists(resolved_source.string())) {
        source_request.query = resolved_source.filename().string();
        std::error_code type_error;
        const bool is_directory = std::filesystem::is_directory(resolved_source, type_error);
        if (!type_error) {
            const auto type = is_directory ? entity_resolution::EntityType::Folder
                                           : entity_resolution::EntityType::File;
            if (source_request.expected_type == entity_resolution::EntityType::Unknown ||
                source_request.expected_type == type) {
                source_request.candidates.push_back({
                    resolved_source.string(), resolved_source.filename().string(), type,
                    resolved_source.parent_path().string(), "linux.filesystem", true, 0
                });
            }
        }
    } else {
        for (const auto& root : roots) {
            std::string list_error;
            const auto entries = platform_.filesystem().list(root.string(), list_error);
            for (const auto& entry : entries) {
                const auto type = entry.directory ? entity_resolution::EntityType::Folder
                                                  : entity_resolution::EntityType::File;
                if (source_request.expected_type != entity_resolution::EntityType::Unknown &&
                    source_request.expected_type != type) continue;
                source_request.candidates.push_back({
                    entry.path, entry.name, type, root.string(),
                    "linux.filesystem", true, 0
                });
            }
        }
    }

    const auto source_match = entity_resolver_.resolve(std::move(source_request));
    entity_resolution::EntityCandidate selected_source;
    if (source_match.status == entity_resolution::ResolutionStatus::Ambiguous) {
        bool invalid_answer = false;
        const auto choice = choose_candidate(
            request, "source choice", source_match.candidates, invalid_answer);
        if (!choice) {
            return needs_choice(request.request_id, "source",
                                source_match.candidates, invalid_answer);
        }
        selected_source = *choice;
    } else if (source_match.status == entity_resolution::ResolutionStatus::Found) {
        selected_source = source_match.selected;
    } else {
        return failure(request.request_id, "The source file or folder could not be found.");
    }

    const auto source_path = std::filesystem::path(selected_source.id);
    std::filesystem::path destination_path;
    std::string capability_id;
    std::string execution_operation;

    if (intent.operation == "rename") {
        const std::filesystem::path new_name(destination_value);
        if (new_name.empty() || new_name.is_absolute() || new_name.has_parent_path() ||
            destination_value == "." || destination_value == "..") {
            return failure(request.request_id, "The new name must be a single path component.");
        }
        destination_path = source_path.parent_path() / new_name;
        capability_id = "filesystem.rename";
        execution_operation = "rename_path";
    } else {
        const auto destination_directory = resolve_destination(destination_value);
        std::error_code ec;
        if (destination_directory.empty() ||
            !std::filesystem::is_directory(destination_directory, ec) || ec) {
            return failure(request.request_id, "The destination directory could not be resolved.");
        }
        destination_path = std::filesystem::path(destination_directory) /
                           source_path.filename();
        capability_id = intent.operation == "copy"
            ? "filesystem.copy" : "filesystem.rename";
        execution_operation = intent.operation == "copy"
            ? "copy_file" : "rename_path";
    }

    permissions::PermissionRequest source_permission;
    source_permission.capability_id = capability_id;
    source_permission.operation = execution_operation;
    source_permission.resource_path = source_path.string();
    source_permission.modifies_state = true;
    const auto source_allowed = permission_manager_.authorize(source_permission);
    if (source_allowed.decision != permissions::PermissionDecision::Allowed) {
        return failure(request.request_id, source_allowed.reason);
    }

    permissions::PermissionRequest destination_permission;
    destination_permission.capability_id = capability_id;
    destination_permission.operation = execution_operation;
    destination_permission.resource_path = destination_path.string();
    destination_permission.modifies_state = true;
    const auto destination_allowed = permission_manager_.authorize(destination_permission);
    if (destination_allowed.decision != permissions::PermissionDecision::Allowed) {
        return failure(request.request_id, destination_allowed.reason);
    }

    planning::PlanningRequest planning_request;
    planning_request.goal = intent.operation + " " + source_path.string() +
                           " to " + destination_path.string();
    planning::TaskDraft draft;
    draft.id = "modify-path";
    draft.operation = execution_operation;
    draft.input_data["source"] = source_path.string();
    draft.input_data["destination"] = destination_path.string();
    planning_request.tasks.push_back(std::move(draft));
    auto plan = planning_engine_.create_plan(planning_request);
    if (!plan.valid || plan.tasks.empty()) {
        return failure(request.request_id, "Unable to create a valid filesystem plan.");
    }
    if (!plan_state_manager_.start(plan, plan.tasks.front().id)) {
        return failure(request.request_id, "The filesystem task could not enter the running state.");
    }

    execution::ExecutionRequest execution_request;
    execution_request.task_id = request.task_id;
    execution_request.cir_id = intent.id;
    execution_request.plan_id = plan.id;
    execution_request.plan_step_id = plan.tasks.front().id;
    execution_request.operation = execution_operation;
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    execution_request.authorized_root = source_allowed.authorized_root;
    execution_request.authorized_destination_root = destination_allowed.authorized_root;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                executed.error + " The plan state could not be marked failed.");
        }
        return failure(request.request_id, executed.error);
    }
    verification::VerificationRequest verification_request;
    verification_request.operation = execution_operation;
    verification_request.expected["destination_exists"] = "true";
    verification_request.actual["destination_exists"] =
        platform_.filesystem().exists(destination_path.string()) ? "true" : "false";
    if (intent.operation != "copy") {
        verification_request.expected["source_exists"] = "false";
        verification_request.actual["source_exists"] =
            platform_.filesystem().exists(source_path.string()) ? "true" : "false";
    } else {
        verification_request.expected["source_exists"] = "true";
        verification_request.actual["source_exists"] =
            platform_.filesystem().exists(source_path.string()) ? "true" : "false";
    }
    const auto verification_result = verification_system_.verify(verification_request);
    (void)execution_engine_.set_verification(request.task_id,
        verification_result.verified ? execution::VerificationOutcome::Passed
                                     : execution::VerificationOutcome::Failed);
    if (!verification_result.verified) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                "The filesystem operation ran, verification failed, and the plan state could not be marked failed.");
        }
        return failure(request.request_id,
            "The filesystem operation ran, but verification did not pass.");
    }

    if (!plan_state_manager_.complete(plan, plan.tasks.front().id)) {
        return failure(request.request_id,
            "Execution succeeded, but the plan state could not be marked complete.");
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = intent.operation;
    result.result_data["entity_type"] = intent.entity_type;
    result.result_data["name"] = destination_path.filename().string();
    result.result_data["source"] = source_path.string();
    result.result_data["destination"] = destination_path.string();
    result.result_data["verified"] = "true";
    if (intent.operation != "copy") {
        auto& session = context_manager_.create("session");
        session.values["last_entity.type"] =
            selected_source.type == entity_resolution::EntityType::Folder ? "folder" : "file";
        session.values["last_entity.name"] = destination_path.filename().string();
        session.values["last_entity.path"] = destination_path.string();
        session.values.erase("last_entity.id");
    }
    return result;
}


SubsystemResult CommandSubsystem::delete_target(
    const SubsystemRequest& request, const grammar::CanonicalIntent& intent) {
    const char* home = std::getenv("HOME");
    if (!home) return failure(request.request_id, "The user's home directory is unavailable.");

    entity_resolution::ResolutionRequest source_request;
    source_request.query = intent.name;
    if (intent.entity_type == "file") {
        source_request.expected_type = entity_resolution::EntityType::File;
    } else if (intent.entity_type == "folder") {
        source_request.expected_type = entity_resolution::EntityType::Folder;
    }

    const std::filesystem::path requested(intent.name);
    std::filesystem::path resolved_path;
    if (requested.is_absolute()) resolved_path = requested;
    else {
        const auto home_path = std::filesystem::path(home) / requested;
        if (platform_.filesystem().exists(home_path.string())) resolved_path = home_path;
    }

    if (!resolved_path.empty() && platform_.filesystem().exists(resolved_path.string())) {
        source_request.query = resolved_path.filename().string();
        std::error_code type_error;
        const bool directory = std::filesystem::is_directory(resolved_path, type_error);
        if (!type_error) {
            const auto type = directory ? entity_resolution::EntityType::Folder
                                        : entity_resolution::EntityType::File;
            if (source_request.expected_type == entity_resolution::EntityType::Unknown ||
                source_request.expected_type == type) {
                source_request.candidates.push_back({
                    resolved_path.string(), resolved_path.filename().string(), type,
                    resolved_path.parent_path().string(), "linux.filesystem", true, 0
                });
            }
        }
    } else {
        const std::vector<std::filesystem::path> roots{
            home,
            std::filesystem::path(home) / "Desktop",
            std::filesystem::path(home) / "Documents",
            std::filesystem::path(home) / "Downloads"
        };
        for (const auto& root : roots) {
            std::string list_error;
            const auto entries = platform_.filesystem().list(root.string(), list_error);
            for (const auto& entry : entries) {
                const auto type = entry.directory ? entity_resolution::EntityType::Folder
                                                  : entity_resolution::EntityType::File;
                if (source_request.expected_type != entity_resolution::EntityType::Unknown &&
                    source_request.expected_type != type) continue;
                source_request.candidates.push_back({
                    entry.path, entry.name, type, root.string(),
                    "linux.filesystem", true, 0
                });
            }
        }
    }

    const auto match = entity_resolver_.resolve(std::move(source_request));
    entity_resolution::EntityCandidate selected_target;
    if (match.status == entity_resolution::ResolutionStatus::Ambiguous) {
        bool invalid_answer = false;
        const auto choice = choose_candidate(
            request, "delete target choice", match.candidates, invalid_answer);
        if (!choice) {
            return needs_choice(request.request_id, "delete target",
                                match.candidates, invalid_answer);
        }
        selected_target = *choice;
    } else if (match.status == entity_resolution::ResolutionStatus::Found) {
        selected_target = match.selected;
    } else {
        return failure(request.request_id, "The file or folder to delete could not be found.");
    }

    const auto path = selected_target.id;
    permissions::PermissionRequest permission_request;
    permission_request.capability_id = "filesystem.delete";
    permission_request.operation = "delete_path";
    permission_request.resource_path = path;
    permission_request.destructive = true;
    permission_request.modifies_state = true;
    const auto confirmation = request.input_data.find("confirmed");
    permission_request.user_confirmed =
        confirmation != request.input_data.end() && confirmation->second == "yes";

    const auto permission = permission_manager_.authorize(permission_request);
    if (permission.decision == permissions::PermissionDecision::RequiresConfirmation) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = SubsystemStatus::NeedsPermission;
        result.error = permission.reason;
        result.result_data["confirmation_action"] = "delete";
        result.result_data["path"] = path;
        return result;
    }
    if (permission.decision != permissions::PermissionDecision::Allowed) {
        return failure(request.request_id, permission.reason);
    }

    planning::PlanningRequest planning_request;
    planning_request.goal = "Delete " + path;
    planning::TaskDraft draft;
    draft.id = "delete-target";
    draft.operation = "delete_path";
    draft.input_data["path"] = path;
    planning_request.tasks.push_back(std::move(draft));
    auto plan = planning_engine_.create_plan(planning_request);
    if (!plan.valid || plan.tasks.empty()) {
        return failure(request.request_id, "Unable to create a valid deletion plan.");
    }
    if (!plan_state_manager_.start(plan, plan.tasks.front().id)) {
        return failure(request.request_id, "The deletion task could not enter the running state.");
    }

    execution::ExecutionRequest execution_request;
    execution_request.task_id = request.task_id;
    execution_request.cir_id = intent.id;
    execution_request.plan_id = plan.id;
    execution_request.plan_step_id = plan.tasks.front().id;
    execution_request.operation = "delete_path";
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    execution_request.authorized_root = permission.authorized_root;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                executed.error + " The plan state could not be marked failed.");
        }
        return failure(request.request_id, executed.error);
    }
    verification::VerificationRequest verification_request;
    verification_request.operation = "delete_path";
    verification_request.expected["exists"] = "false";
    verification_request.actual["exists"] =
        platform_.filesystem().exists(path) ? "true" : "false";
    const auto verification_result = verification_system_.verify(verification_request);
    (void)execution_engine_.set_verification(request.task_id,
        verification_result.verified ? execution::VerificationOutcome::Passed
                                     : execution::VerificationOutcome::Failed);
    if (!verification_result.verified) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                "The delete action ran, verification failed, and the plan state could not be marked failed.");
        }
        return failure(request.request_id, "The delete action ran, but verification did not pass.");
    }

    if (!plan_state_manager_.complete(plan, plan.tasks.front().id)) {
        return failure(request.request_id,
            "Execution succeeded, but the plan state could not be marked complete.");
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "delete";
    result.result_data["entity_type"] = intent.entity_type;
    result.result_data["name"] = std::filesystem::path(path).filename().string();
    result.result_data["path"] = path;
    result.result_data["verified"] = "true";
    result.result_data["completed_steps"] = "1";
    result.result_data["attempted_steps"] = "1";
    result.result_data["total_steps"] = "1";
    if (const auto record = execution_engine_.record_of(request.task_id)) {
        result.result_data["execution_record_ids"] = record->record_id;
    }
    return result;
}


SubsystemResult CommandSubsystem::count_file(
    const SubsystemRequest& request, const grammar::CanonicalIntent& intent) {
    const char* home = std::getenv("HOME");
    if (!home) return failure(request.request_id, "The user's home directory is unavailable.");

    std::string metric = lower(intent.query);
    std::string operation;
    if (metric == "word" || metric == "words") operation = "word_count";
    else if (metric == "line" || metric == "lines") operation = "line_count";
    else if (metric == "character" || metric == "characters") operation = "character_count";
    else return failure(request.request_id,
        "Choose a supported count: words, lines, or characters.");

    std::string requested_name = intent.name;
    const auto reference = lower(requested_name);
    if (reference == "it" || reference == "that" || reference == "that file" ||
        reference == "previous file") {
        if (const auto* session = context_manager_.find("session")) {
            const auto last_path = session->values.find("last_entity.path");
            const auto last_type = session->values.find("last_entity.type");
            if (last_path != session->values.end() && last_type != session->values.end() &&
                last_type->second == "file") {
                requested_name = last_path->second;
            }
        }
    }

    entity_resolution::ResolutionRequest file_request;
    file_request.query = requested_name;
    file_request.expected_type = entity_resolution::EntityType::File;
    const std::filesystem::path requested(requested_name);
    std::filesystem::path resolved_path;
    if (requested.is_absolute()) resolved_path = requested;
    else {
        const auto home_path = std::filesystem::path(home) / requested;
        if (platform_.filesystem().exists(home_path.string())) resolved_path = home_path;
    }

    if (!resolved_path.empty() && platform_.filesystem().exists(resolved_path.string())) {
        file_request.query = resolved_path.filename().string();
        std::error_code type_error;
        if (std::filesystem::is_regular_file(resolved_path, type_error) && !type_error) {
            file_request.candidates.push_back({
                resolved_path.string(), resolved_path.filename().string(),
                entity_resolution::EntityType::File,
                resolved_path.parent_path().string(), "linux.filesystem", true, 0
            });
        }
    } else {
        const std::vector<std::filesystem::path> roots{
            home,
            std::filesystem::path(home) / "Desktop",
            std::filesystem::path(home) / "Documents",
            std::filesystem::path(home) / "Downloads"
        };
        for (const auto& root : roots) {
            std::string list_error;
            const auto entries = platform_.filesystem().list(root.string(), list_error);
            for (const auto& entry : entries) {
                if (entry.directory) continue;
                file_request.candidates.push_back({
                    entry.path, entry.name, entity_resolution::EntityType::File,
                    root.string(), "linux.filesystem", true, 0
                });
            }
        }
    }

    const auto match = entity_resolver_.resolve(std::move(file_request));
    entity_resolution::EntityCandidate selected_file;
    if (match.status == entity_resolution::ResolutionStatus::Ambiguous) {
        bool invalid_answer = false;
        const auto choice = choose_candidate(
            request, "file choice", match.candidates, invalid_answer);
        if (!choice) {
            return needs_choice(request.request_id, "file",
                                match.candidates, invalid_answer);
        }
        selected_file = *choice;
    } else if (match.status == entity_resolution::ResolutionStatus::Found) {
        selected_file = match.selected;
    } else {
        return failure(request.request_id, "The text file could not be found.");
    }

    const auto path = selected_file.id;
    permissions::PermissionRequest permission_request;
    permission_request.capability_id = "filesystem.read_text";
    permission_request.operation = "read_text";
    permission_request.resource_path = path;
    const auto permission = permission_manager_.authorize(permission_request);
    if (permission.decision != permissions::PermissionDecision::Allowed) {
        return failure(request.request_id, permission.reason);
    }

    planning::PlanningRequest planning_request;
    planning_request.goal = "Count " + metric + " in " + path;
    planning::TaskDraft draft;
    draft.id = "count-file-text";
    draft.operation = "read_text";
    draft.input_data["path"] = path;
    planning_request.tasks.push_back(std::move(draft));
    auto plan = planning_engine_.create_plan(planning_request);
    if (!plan.valid || plan.tasks.empty()) {
        return failure(request.request_id, "Unable to create a valid text-analysis plan.");
    }
    if (!plan_state_manager_.start(plan, plan.tasks.front().id)) {
        return failure(request.request_id, "The text-analysis task could not start.");
    }

    execution::ExecutionRequest execution_request;
    execution_request.task_id = request.task_id;
    execution_request.cir_id = intent.id;
    execution_request.plan_id = plan.id;
    execution_request.plan_step_id = plan.tasks.front().id;
    execution_request.operation = "read_text";
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    execution_request.authorized_root = permission.authorized_root;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                executed.error + " The plan state could not be marked failed.");
        }
        return failure(request.request_id, executed.error);
    }
    const auto text_value = executed.output_data.find("text");
    if (text_value == executed.output_data.end()) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                "The file reader returned no text content and the plan state could not be marked failed.");
        }
        return failure(request.request_id, "The file reader returned no text content.");
    }
    if (text_value->second.find('\0') != std::string::npos) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                "The selected file appears to contain binary data and the plan state could not be marked failed.");
        }
        return failure(request.request_id, "The selected file appears to contain binary data.");
    }

    information::InformationProcessor processor(information_registry_);
    information::DataRequest data_request;
    data_request.operation = operation;
    data_request.text_input = text_value->second;
    const auto count = processor.process(data_request);
    if (!count.success) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                count.error + " The plan state could not be marked failed.");
        }
        return failure(request.request_id, count.error);
    }

    verification::VerificationRequest verification_request;
    verification_request.operation = "count_file_text";
    verification_request.expected["exists"] = "true";
    verification_request.actual["exists"] =
        platform_.filesystem().exists(path) ? "true" : "false";
    const auto verification_result = verification_system_.verify(verification_request);
    (void)execution_engine_.set_verification(request.task_id,
        verification_result.verified ? execution::VerificationOutcome::Passed
                                     : execution::VerificationOutcome::Failed);
    if (!verification_result.verified) {
        if (!plan_state_manager_.fail(plan, plan.tasks.front().id)) {
            return failure(request.request_id,
                "The file could not be verified after reading, and the plan state could not be marked failed.");
        }
        return failure(request.request_id, "The file could not be verified after reading.");
    }

    if (!plan_state_manager_.complete(plan, plan.tasks.front().id)) {
        return failure(request.request_id,
            "Execution succeeded, but the plan state could not be marked complete.");
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "count";
    result.result_data["entity_type"] = "file";
    result.result_data["name"] = std::filesystem::path(path).filename().string();
    result.result_data["path"] = path;
    result.result_data["metric"] = metric;
    result.result_data["value"] = count.scalar;
    result.result_data["bytes"] = executed.output_data.contains("bytes")
        ? executed.output_data.at("bytes") : "0";
    auto& session = context_manager_.create("session");
    session.values["last_entity.type"] = "file";
    session.values["last_entity.name"] = result.result_data["name"];
    session.values["last_entity.path"] = path;
    session.values.erase("last_entity.id");
    return result;
}


SubsystemResult CommandSubsystem::create_folder_chain(
    const SubsystemRequest& request, const std::vector<std::string>& clauses) {
    if (clauses.size() < 2) return handle_single(request, clauses.front());

    planning::PlanningRequest planning_request;
    planning_request.goal = "Execute a dependent filesystem operation sequence";
    std::vector<grammar::CanonicalIntent> intents;
    std::vector<std::string> cir_ids;
    intents.reserve(clauses.size());
    cir_ids.reserve(clauses.size());

    // Validate the dependency graph before resolving live filesystem paths so
    // preflight failures can still be tied to a stable plan.
    for (std::size_t index = 0; index < clauses.size(); ++index) {
        auto intent = grammar_engine_.interpret(clauses[index]);
        cir_ids.push_back(intent.id);
        if (!intent.valid) {
            return failure(request.request_id,
                intent.error.empty() ? "A sequence step could not be understood." : intent.error);
        }
        planning::TaskDraft draft;
        draft.id = "filesystem-step-" + std::to_string(index + 1);
        if (index > 0) draft.dependencies.push_back("filesystem-step-" + std::to_string(index));
        if (intent.operation == "create" && intent.entity_type == "folder") {
            draft.operation = "create_directory";
        } else if (intent.operation == "rename") {
            draft.operation = "rename_path";
        } else {
            return failure(request.request_id,
                "Multi-step v0.1 supports folder creation and renaming folders created earlier in the sequence.");
        }
        planning_request.tasks.push_back(std::move(draft));
        intents.push_back(std::move(intent));
    }

    auto plan = planning_engine_.create_plan(planning_request);
    if (!plan.valid || plan.tasks.size() != clauses.size()) {
        return failure(request.request_id,
            plan.errors.empty() ? "The multi-step filesystem plan could not be validated."
                                : plan.errors.front());
    }

    const auto fail_preflight = [&](std::size_t failed_index, const std::string& message) {
        std::vector<execution::ExecutionStepRecord> steps;
        steps.reserve(failed_index + 1);
        for (std::size_t index = 0; index <= failed_index && index < plan.tasks.size(); ++index) {
            execution::ExecutionStepRecord step;
            step.step_id = plan.tasks[index].id;
            step.operation = plan.tasks[index].operation;
            step.input_data = plan.tasks[index].input_data;
            step.outcome = index == failed_index
                ? execution::StepOutcome::Failed : execution::StepOutcome::Attempted;
            if (index == failed_index) step.error = message;
            steps.push_back(std::move(step));
        }
        (void)execution_engine_.record_terminal_failure(
            request.task_id, steps, cir_ids.empty() ? std::string{} : cir_ids.front(),
            plan.id, false);
        return failure(request.request_id, message);
    };

    std::map<std::string, std::string> local_paths;
    std::vector<std::string> target_paths;
    std::vector<std::string> folder_names;
    std::vector<std::shared_ptr<const permissions::AuthorizedRoot>> authorized_roots;
    std::vector<std::shared_ptr<const permissions::AuthorizedRoot>> destination_roots;
    target_paths.reserve(clauses.size());
    folder_names.reserve(clauses.size());
    authorized_roots.reserve(clauses.size());
    destination_roots.reserve(clauses.size());
    std::string last_created_path;

    for (std::size_t index = 0; index < intents.size(); ++index) {
        const auto& intent = intents[index];
        auto& draft = plan.tasks[index];
        std::string target;
        std::string display_name;

        if (intent.operation == "create" && intent.entity_type == "folder") {
            const std::filesystem::path folder_name(intent.name);
            if (folder_name.empty() || folder_name.is_absolute() ||
                folder_name.has_parent_path() || intent.name == "." || intent.name == "..") {
                return fail_preflight(index, "A folder name must be a single path component.");
            }
            if (intent.destination.empty()) {
                return fail_preflight(index, "Every folder-creation step needs a destination.");
            }

            // Preserve the parser's raw slots in the plan so a preflight
            // failure can be diagnosed through "inspect task".
            draft.input_data["name"] = intent.name;
            draft.input_data["destination"] = intent.destination;

            // Grammar commonly captures relational wording ("inside it",
            // "in Projects") as part of the destination slot. Resolve that
            // relation separately from the entity name used as the map key.
            auto destination_reference = entity_key(intent.destination);
            for (const std::string_view prefix : {"inside ", "within ", "in "}) {
                if (destination_reference.rfind(prefix, 0) == 0) {
                    destination_reference = entity_key(
                        destination_reference.substr(prefix.size()));
                    break;
                }
            }

            std::string destination;
            bool planned_destination = false;
            if (destination_reference == "it" || destination_reference == "that" ||
                destination_reference == "that folder" ||
                destination_reference == "previous folder") {
                destination = last_created_path;
                planned_destination = !destination.empty();
            } else {
                const auto local = local_paths.find(destination_reference);
                if (local != local_paths.end()) {
                    destination = local->second;
                    planned_destination = true;
                } else {
                    // Resolve ordinary locations such as Desktop after
                    // removing relational wording and trailing punctuation.
                    destination = resolve_destination(destination_reference);
                }
            }

            draft.input_data["destination_reference"] = destination_reference;
            draft.input_data["last_created_path"] = last_created_path;
            draft.input_data["planned_destination"] = planned_destination ? "true" : "false";
            draft.input_data["resolved_destination"] = destination;

            std::error_code ec;
            if (destination.empty() ||
                (!planned_destination &&
                 (!std::filesystem::is_directory(destination, ec) || ec))) {
                return fail_preflight(index,
                    "A folder destination does not exist or could not be resolved.");
            }
            target = (std::filesystem::path(destination) / folder_name).lexically_normal().string();

            reasoning::Fact existence_fact;
            existence_fact.subject = target;
            existence_fact.property = "exists";
            existence_fact.value = platform_.filesystem().exists(target) ? "true" : "false";
            existence_fact.source = "linux.filesystem";
            existence_fact.confidence = 100;
            existence_fact.fresh = true;
            reasoning::ReasoningRequest reasoning_request;
            reasoning_request.query = {target, "exists", "false", true};
            reasoning_request.facts.push_back(std::move(existence_fact));
            const auto reasoning_result = reasoning_engine_.evaluate(reasoning_request);
            if (reasoning_result.truth != TruthValue::True) {
                return fail_preflight(index,
                    reasoning_result.truth == TruthValue::False
                        ? "A target folder with that name already exists."
                        : "The system could not determine whether the target already exists.");
            }

            permissions::PermissionRequest permission;
            permission.capability_id = "filesystem.create_directory";
            permission.operation = "create_directory";
            permission.resource_path = target;
            permission.modifies_state = true;
            const auto allowed = permission_manager_.authorize(permission);
            if (allowed.decision != permissions::PermissionDecision::Allowed) {
                return fail_preflight(index, allowed.reason);
            }
            authorized_roots.push_back(allowed.authorized_root);
            destination_roots.push_back(nullptr);

            draft.input_data["path"] = target;
            display_name = intent.name;
            local_paths[entity_key(intent.name)] = target;
        } else {
            const auto source_name = entity_key(intent.source);
            std::string source;
            if (source_name == "it" || source_name == "that" ||
                source_name == "that folder" || source_name == "previous folder") {
                source = last_created_path;
            } else {
                const auto local = local_paths.find(source_name);
                if (local != local_paths.end()) source = local->second;
            }
            if (source.empty()) {
                return fail_preflight(index,
                    "A rename step must refer to a folder created earlier in the sequence.");
            }

            const std::filesystem::path new_name(intent.destination);
            if (new_name.empty() || new_name.is_absolute() || new_name.has_parent_path() ||
                intent.destination == "." || intent.destination == "..") {
                return fail_preflight(index,
                    "The new folder name must be a single path component.");
            }
            target = (std::filesystem::path(source).parent_path() / new_name).lexically_normal().string();
            if (platform_.filesystem().exists(target)) {
                return fail_preflight(index, "The rename destination already exists.");
            }

            permissions::PermissionRequest permission;
            permission.capability_id = "filesystem.rename";
            permission.operation = "rename_path";
            permission.resource_path = source;
            permission.modifies_state = true;
            const auto source_allowed = permission_manager_.authorize(permission);
            if (source_allowed.decision != permissions::PermissionDecision::Allowed) {
                return fail_preflight(index, source_allowed.reason);
            }
            permission.resource_path = target;
            const auto destination_allowed = permission_manager_.authorize(permission);
            if (destination_allowed.decision != permissions::PermissionDecision::Allowed) {
                return fail_preflight(index, destination_allowed.reason);
            }
            authorized_roots.push_back(source_allowed.authorized_root);
            destination_roots.push_back(destination_allowed.authorized_root);

            draft.input_data["source"] = source;
            draft.input_data["destination"] = target;
            display_name = intent.destination;
            local_paths[entity_key(intent.destination)] = target;
        }

        target_paths.push_back(target);
        folder_names.push_back(display_name);
        last_created_path = target;
    }

    std::size_t completed = 0;
    std::vector<std::string> execution_record_ids;
    const auto joined_execution_record_ids = [&execution_record_ids]() {
        std::string joined;
        for (const auto& id : execution_record_ids) {
            if (!joined.empty()) joined += '\n';
            joined += id;
        }
        return joined;
    };
    for (std::size_t index = 0; index < plan.tasks.size(); ++index) {
        if (request.cancellation_requested &&
            request.cancellation_requested->load()) {
            auto result = failure(request.request_id,
                "Task cancelled between execution steps; completed work was left intact.");
            result.result_data["cancelled"] = "true";
            result.result_data["completed_steps"] = std::to_string(completed);
            result.result_data["total_steps"] = std::to_string(clauses.size());
            result.result_data["attempted_steps"] = std::to_string(execution_record_ids.size());
            result.result_data["execution_record_ids"] = joined_execution_record_ids();
            return result;
        }
        auto& task = plan.tasks[index];
        if (!plan_state_manager_.start(plan, task.id)) {
            auto result = failure(request.request_id,
                "A dependent filesystem task could not start.");
            result.result_data["completed_steps"] = std::to_string(completed);
            result.result_data["total_steps"] = std::to_string(clauses.size());
            result.result_data["attempted_steps"] = std::to_string(execution_record_ids.size());
            result.result_data["execution_record_ids"] = joined_execution_record_ids();
            return result;
        }

        execution::ExecutionRequest execution_request;
        execution_request.task_id = request.task_id + "-chain-" + std::to_string(index + 1);
        execution_request.cir_id = cir_ids[index];
        execution_request.plan_id = plan.id;
        execution_request.plan_step_id = task.id;
        execution_request.operation = task.operation;
        execution_request.input_data = task.input_data;
        execution_request.requires_authorization = true;
        execution_request.authorization_granted = true;
        execution_request.authorized_root = authorized_roots[index];
        execution_request.authorized_destination_root = destination_roots[index];
        const auto executed = execution_engine_.execute(execution_request);
        execution_record_ids.push_back(execution_request.task_id);
        if (executed.status != execution::ExecutionStatus::Success) {
            const bool marked_failed = plan_state_manager_.fail(plan, task.id);
            auto result = failure(request.request_id, executed.error);
            if (!marked_failed) result.error += " The plan state could not be marked failed.";
            result.result_data["completed_steps"] = std::to_string(completed);
            result.result_data["total_steps"] = std::to_string(clauses.size());
            result.result_data["attempted_steps"] = std::to_string(execution_record_ids.size());
            result.result_data["execution_record_ids"] = joined_execution_record_ids();
            return result;
        }

        verification::VerificationRequest verification;
        verification.operation = task.operation;
        if (task.operation == "create_directory") {
            verification.expected["exists"] = "true";
            verification.actual["exists"] =
                platform_.filesystem().exists(target_paths[index]) ? "true" : "false";
        } else {
            verification.expected["destination_exists"] = "true";
            verification.actual["destination_exists"] =
                platform_.filesystem().exists(target_paths[index]) ? "true" : "false";
            verification.expected["source_exists"] = "false";
            verification.actual["source_exists"] =
                platform_.filesystem().exists(task.input_data.at("source")) ? "true" : "false";
        }
        const auto verification_result = verification_system_.verify(verification);
        (void)execution_engine_.set_verification(execution_request.task_id,
            verification_result.verified ? execution::VerificationOutcome::Passed
                                         : execution::VerificationOutcome::Failed);
        if (!verification_result.verified) {
            const bool marked_failed = plan_state_manager_.fail(plan, task.id);
            auto result = failure(request.request_id,
                "A filesystem operation ran but its verification did not pass.");
            if (!marked_failed) result.error += " The plan state could not be marked failed.";
            result.result_data["completed_steps"] = std::to_string(completed);
            result.result_data["total_steps"] = std::to_string(clauses.size());
            result.result_data["attempted_steps"] = std::to_string(execution_record_ids.size());
            result.result_data["execution_record_ids"] = joined_execution_record_ids();
            return result;
        }
        if (!plan_state_manager_.complete(plan, task.id)) {
            auto result = failure(request.request_id,
                "An operation was verified, but its plan state could not be completed.");
            result.result_data["completed_steps"] = std::to_string(completed + 1);
            result.result_data["total_steps"] = std::to_string(clauses.size());
            result.result_data["attempted_steps"] = std::to_string(execution_record_ids.size());
            result.result_data["execution_record_ids"] = joined_execution_record_ids();
            return result;
        }
        remember_folder(folder_names[index], target_paths[index]);
        ++completed;
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "filesystem_chain";
    result.result_data["entity_type"] = "folder";
    result.result_data["count"] = std::to_string(completed);
    std::string joined_paths;
    for (const auto& path : target_paths) {
        if (!joined_paths.empty()) joined_paths += '\n';
        joined_paths += path;
    }
    result.result_data["paths"] = std::move(joined_paths);
    result.result_data["path"] = target_paths.back();
    result.result_data["name"] = folder_names.back();
    result.result_data["verified"] = "true";
    result.result_data["execution_record_ids"] = joined_execution_record_ids();
    return result;
}

SubsystemResult CommandSubsystem::handle_single(const SubsystemRequest& request, const std::string& raw_input) {
    if (raw_input.empty()) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = SubsystemStatus::NeedsInformation;
        result.missing_information.emplace_back("instruction text");
        return result;
    }

    auto intent = grammar_engine_.interpret(raw_input);

    // Reapply answers collected while this task was paused before deciding
    // whether the original instruction is still incomplete.
    bool applied_clarification = false;
    for (const auto& [field, answer] : request.context) {
        constexpr const char* prefix = "clarification.";
        if (field.rfind(prefix, 0) != 0 || answer.empty()) continue;
        const auto slot = field.substr(std::char_traits<char>::length(prefix));
        intent.slots[slot] = grammar::SlotValue{answer, false, 0, 0};
        applied_clarification = true;
        if (slot == "name" || slot == "target" || slot == "file") {
            intent.name = answer;
        } else if (slot == "source") {
            intent.source = answer;
        } else if (slot == "destination") {
            intent.destination = answer;
        } else if (slot == "query") {
            intent.query = answer;
        }
        intent.missing_information.erase(
            std::remove(intent.missing_information.begin(),
                        intent.missing_information.end(), slot),
            intent.missing_information.end());
    }
    if (applied_clarification && intent.missing_information.empty() &&
        !intent.operation.empty()) {
        intent.valid = true;
        intent.error.clear();
    }

    if (!intent.valid) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = intent.missing_information.empty()
            ? SubsystemStatus::Failure : SubsystemStatus::NeedsInformation;
        result.missing_information = intent.missing_information;
        result.error = intent.error;
        return result;
    }

    if (intent.operation == "open" &&
        (intent.entity_type == "application" || intent.entity_type == "folder")) {
        return open_target(request, intent);
    }
    if (intent.operation == "create" && intent.entity_type == "folder") {
        return create_folder(request, intent);
    }
    if (intent.operation == "search") {
        return search_items(request, intent);
    }
    if (intent.operation == "rename" || intent.operation == "move" ||
        intent.operation == "copy") {
        return modify_path(request, intent);
    }
    if (intent.operation == "delete") {
        return delete_target(request, intent);
    }
    if (intent.operation == "count") {
        return count_file(request, intent);
    }
    return failure(request.request_id,
        "The current end-to-end workflow supports folder creation, opening applications or folders, scoped directory searches, rename, move, file copy, confirmed deletion of files or empty folders, and text-file counts.");
}


SubsystemResult CommandSubsystem::handle(const SubsystemRequest& request) {
    const auto raw = request.input_data.find("raw_input");
    if (raw == request.input_data.end() || raw->second.empty()) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = SubsystemStatus::NeedsInformation;
        result.missing_information.emplace_back("instruction text");
        return result;
    }

    const auto normalized = lower(raw->second);
    const bool inspect_latest = normalized == "show last task";
    constexpr std::string_view inspect_prefix{"inspect task "};
    if (inspect_latest || normalized.rfind(inspect_prefix, 0) == 0 ||
        normalized == "inspect task") {
        std::optional<execution::ExecutionRecord> record;
        if (inspect_latest) {
            record = execution_engine_.latest_record();
        } else if (normalized.size() > inspect_prefix.size()) {
            const auto task_id = raw->second.substr(inspect_prefix.size());
            const auto first = task_id.find_first_not_of(" \t");
            const auto last = task_id.find_last_not_of(" \t");
            if (first != std::string::npos) {
                record = execution_engine_.record_of(task_id.substr(first, last - first + 1));
            }
        }
        if (!record) {
            return failure(request.request_id, inspect_latest
                ? "There is no execution record in this session yet."
                : "That execution record was not found. Check the task ID; records are session-only.");
        }

        const auto status_name = [](execution::ExecutionStatus status) {
            switch (status) {
            case execution::ExecutionStatus::Success: return "Success";
            case execution::ExecutionStatus::NeedsPermission: return "Blocked";
            case execution::ExecutionStatus::Unsupported: return "Unsupported";
            case execution::ExecutionStatus::Failure: return "Failure";
            }
            return "Unknown";
        };
        const auto outcome_name = [](execution::StepOutcome outcome) {
            switch (outcome) {
            case execution::StepOutcome::Attempted: return "Attempted";
            case execution::StepOutcome::Succeeded: return "Succeeded";
            case execution::StepOutcome::Blocked: return "Blocked";
            case execution::StepOutcome::Failed: return "Failed";
            }
            return "Unknown";
        };
        const auto verification_name = [](execution::VerificationOutcome outcome) {
            switch (outcome) {
            case execution::VerificationOutcome::NotRun: return "Not performed";
            case execution::VerificationOutcome::Passed: return "Passed";
            case execution::VerificationOutcome::Failed: return "Failed";
            }
            return "Unknown";
        };

        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = SubsystemStatus::Success;
        result.result_data["inspection"] = "true";
        result.result_data["record_task_id"] = record->task_id;
        result.result_data["record_status"] = status_name(record->status);
        result.result_data["cir_id"] = record->cir_id.empty() ? "Not available" : record->cir_id;
        result.result_data["plan_id"] = record->plan_id.empty() ? "Not available" : record->plan_id;
        result.result_data["step_count"] = std::to_string(record->steps.size());

        for (std::size_t index = 0; index < record->steps.size(); ++index) {
            const auto& step = record->steps[index];
            const auto prefix = "step_" + std::to_string(index + 1) + "_";
            result.result_data[prefix + "operation"] = step.operation;
            result.result_data[prefix + "outcome"] = outcome_name(step.outcome);
            result.result_data[prefix + "verification"] = verification_name(step.verification);
            if (!step.error.empty()) result.result_data[prefix + "error"] = step.error;
            for (const auto* key : {"path", "destination", "count", "directory",
                                    "query", "bytes", "displayed", "truncated", "name",
                                    "destination_reference", "last_created_path",
                                    "planned_destination", "resolved_destination"}) {
                auto value = step.output_data.find(key);
                if (value != step.output_data.end() && !value->second.empty()) {
                    result.result_data[prefix + key] = value->second;
                    continue;
                }
                const auto input = step.input_data.find(key);
                if (input != step.input_data.end() && !input->second.empty()) {
                    result.result_data[prefix + key] = input->second;
                }
            }
        }
        return result;
    }

    const auto clauses = split_instruction_chain(raw->second);
    auto result = clauses.size() > 1
        ? create_folder_chain(request, clauses)
        : handle_single(request, raw->second);

    if (result.status == SubsystemStatus::Failure ||
        result.status == SubsystemStatus::NeedsPermission) {
        const auto intent = grammar_engine_.interpret(raw->second);
        const auto operation = intent.operation.empty() ? "command" : intent.operation;
        (void)execution_engine_.record_terminal_failure(
            request.task_id, operation,
            result.error.empty() ? "The command was blocked before execution." : result.error,
            intent.id, result.status == SubsystemStatus::NeedsPermission);
    }
    return result;
}
} // namespace seventh
