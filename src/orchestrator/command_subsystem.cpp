#include "seventh/orchestrator/command_subsystem.hpp"

#include "seventh/platform/linux/linux_operation_executor.hpp"
#include "seventh/systems/entity_resolution/entity_types.hpp"
#include "seventh/systems/permissions/permission_types.hpp"
#include "seventh/systems/verification/verification_types.hpp"
#include "seventh/systems/information/information_processor.hpp"
#include "seventh/systems/information/provider_registry.hpp"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <memory>
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

SubsystemResult failure(const RequestId& id, std::string message) {
    SubsystemResult result;
    result.request_id = id;
    result.status = SubsystemStatus::Failure;
    result.error = std::move(message);
    return result;
}

SubsystemResult needs_choice(const RequestId& id, const std::string& kind) {
    SubsystemResult result;
    result.request_id = id;
    result.status = SubsystemStatus::NeedsInformation;
    result.missing_information.emplace_back(kind + " choice");
    result.error = "More than one matching " + kind + " was found.";
    return result;
}
std::vector<std::string> split_instruction_chain(const std::string& input) {
    const auto lowered = lower(input);
    std::vector<std::string> clauses;
    std::size_t start = 0;
    while (start < input.size()) {
        const auto and_then = lowered.find(" and then ", start);
        const auto then = lowered.find(" then ", start);
        std::size_t position = std::string::npos;
        std::size_t delimiter_size = 0;
        if (and_then != std::string::npos &&
            (then == std::string::npos || and_then <= then)) {
            position = and_then;
            delimiter_size = 10;
        } else if (then != std::string::npos) {
            position = then;
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
    platform::PlatformInterface& platform, grammar::GrammarConfig grammar_config)
    : platform_(platform), grammar_engine_(std::move(grammar_config)),
      permission_manager_(capabilities_) {
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
    if (!home || destination.empty()) return {};

    const std::filesystem::path requested(destination);
    if (requested.is_absolute()) return requested.lexically_normal().string();
    if (lower(destination) == "home") return std::filesystem::path(home).string();
    if (lower(destination) == "desktop") {
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
    if (intent.destination.empty()) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = SubsystemStatus::NeedsInformation;
        result.missing_information.emplace_back("destination folder");
        result.error = "Specify where the new folder should be created.";
        return result;
    }

    const std::filesystem::path folder_name(intent.name);
    if (folder_name.empty() || folder_name.is_absolute() ||
        folder_name.has_parent_path() || intent.name == "." || intent.name == "..") {
        return failure(request.request_id,
            "A folder name must be a single name, not a path.");
    }

    const auto destination_path = resolve_destination(intent.destination);
    if (destination_path.empty()) {
        return failure(request.request_id, "The user's home directory is unavailable.");
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
    execution_request.operation = plan.tasks.front().operation;
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        plan_state_manager_.fail(plan, plan.tasks.front().id);
        return failure(request.request_id, executed.error);
    }
    plan_state_manager_.complete(plan, plan.tasks.front().id);

    verification::VerificationRequest verification_request;
    verification_request.operation = "create_directory";
    verification_request.expected["exists"] = "true";
    verification_request.actual["exists"] =
        platform_.filesystem().exists(target_path) ? "true" : "false";
    if (!verification_system_.verify(verification_request).verified) {
        return failure(request.request_id,
            "The operation ran, but verification could not confirm the new folder.");
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

    if (wanted == "browser") {
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
            return needs_choice(request.request_id, "folder");
        }
        if (folder_match.status == entity_resolution::ResolutionStatus::Found) {
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
                return needs_choice(request.request_id, "application");
            }
            if (app_match.status == entity_resolution::ResolutionStatus::Found) {
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
    execution_request.operation = operation;
    execution_request.input_data = operation_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        plan_state_manager_.fail(plan, plan.tasks.front().id);
        return failure(request.request_id, executed.error);
    }
    plan_state_manager_.complete(plan, plan.tasks.front().id);

    verification::VerificationRequest verification_request;
    verification_request.operation = operation;
    verification_request.expected["started"] = "true";
    verification_request.actual["started"] =
        executed.output_data.contains("started") ? executed.output_data.at("started") : "false";
    if (!verification_system_.verify(verification_request).verified) {
        return failure(request.request_id, "The open request could not be verified.");
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "open";
    result.result_data["entity_type"] = entity_type;
    result.result_data["name"] = resolved_name;
    if (operation == "open_path") result.result_data["path"] = operation_data["path"];
    return result;
}


SubsystemResult CommandSubsystem::search_items(
    const SubsystemRequest& request, const grammar::CanonicalIntent& intent) {
    const char* home = std::getenv("HOME");
    if (!home) return failure(request.request_id, "The user's home directory is unavailable.");

    const auto directory = intent.destination.empty()
        ? std::string(home) : resolve_destination(intent.destination);
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
    execution_request.operation = "search_files";
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        plan_state_manager_.fail(plan, plan.tasks.front().id);
        return failure(request.request_id, executed.error);
    }
    plan_state_manager_.complete(plan, plan.tasks.front().id);

    verification::VerificationRequest verification_request;
    verification_request.operation = "search_files";
    verification_request.expected["directory"] = directory;
    verification_request.actual["directory"] = executed.output_data.contains("directory")
        ? executed.output_data.at("directory") : "";
    if (!verification_system_.verify(verification_request).verified) {
        return failure(request.request_id, "The search result could not be verified.");
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "search";
    result.result_data["entity_type"] = intent.entity_type;
    result.result_data["directory"] = directory;
    result.result_data["query"] = intent.query;
    result.result_data["count"] = executed.output_data.contains("count")
        ? executed.output_data.at("count") : "0";
    result.result_data["matches"] = executed.output_data.contains("matches")
        ? executed.output_data.at("matches") : "";
    return result;
}


SubsystemResult CommandSubsystem::modify_path(
    const SubsystemRequest& request, const grammar::CanonicalIntent& intent) {
    const char* home = std::getenv("HOME");
    if (!home) return failure(request.request_id, "The user's home directory is unavailable.");

    const std::vector<std::filesystem::path> roots{
        home,
        std::filesystem::path(home) / "Desktop",
        std::filesystem::path(home) / "Documents",
        std::filesystem::path(home) / "Downloads"
    };

    entity_resolution::ResolutionRequest source_request;
    source_request.query = intent.source;
    if (intent.entity_type == "file") {
        source_request.expected_type = entity_resolution::EntityType::File;
    } else if (intent.entity_type == "folder") {
        source_request.expected_type = entity_resolution::EntityType::Folder;
    }

    const std::filesystem::path source_as_path(intent.source);
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
    if (source_match.status == entity_resolution::ResolutionStatus::Ambiguous) {
        return needs_choice(request.request_id, "source");
    }
    if (source_match.status != entity_resolution::ResolutionStatus::Found) {
        return failure(request.request_id, "The source file or folder could not be found.");
    }

    const auto source_path = std::filesystem::path(source_match.selected.id);
    std::filesystem::path destination_path;
    std::string capability_id;
    std::string execution_operation;

    if (intent.operation == "rename") {
        const std::filesystem::path new_name(intent.destination);
        if (new_name.empty() || new_name.is_absolute() || new_name.has_parent_path() ||
            intent.destination == "." || intent.destination == "..") {
            return failure(request.request_id, "The new name must be a single path component.");
        }
        destination_path = source_path.parent_path() / new_name;
        capability_id = "filesystem.rename";
        execution_operation = "rename_path";
    } else {
        const auto destination_directory = resolve_destination(intent.destination);
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
    const auto source_allowed = permission_manager_.authorize(source_permission);
    if (source_allowed.decision != permissions::PermissionDecision::Allowed) {
        return failure(request.request_id, source_allowed.reason);
    }

    permissions::PermissionRequest destination_permission;
    destination_permission.capability_id = capability_id;
    destination_permission.operation = execution_operation;
    destination_permission.resource_path = destination_path.string();
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
    execution_request.operation = execution_operation;
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        plan_state_manager_.fail(plan, plan.tasks.front().id);
        return failure(request.request_id, executed.error);
    }
    plan_state_manager_.complete(plan, plan.tasks.front().id);

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
    if (!verification_system_.verify(verification_request).verified) {
        return failure(request.request_id,
            "The filesystem operation ran, but verification did not pass.");
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
    if (match.status == entity_resolution::ResolutionStatus::Ambiguous) {
        return needs_choice(request.request_id, "delete target");
    }
    if (match.status != entity_resolution::ResolutionStatus::Found) {
        return failure(request.request_id, "The file or folder to delete could not be found.");
    }

    const auto path = match.selected.id;
    permissions::PermissionRequest permission_request;
    permission_request.capability_id = "filesystem.delete";
    permission_request.operation = "delete_path";
    permission_request.resource_path = path;
    permission_request.destructive = true;
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
    execution_request.operation = "delete_path";
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        plan_state_manager_.fail(plan, plan.tasks.front().id);
        return failure(request.request_id, executed.error);
    }
    plan_state_manager_.complete(plan, plan.tasks.front().id);

    verification::VerificationRequest verification_request;
    verification_request.operation = "delete_path";
    verification_request.expected["exists"] = "false";
    verification_request.actual["exists"] =
        platform_.filesystem().exists(path) ? "true" : "false";
    if (!verification_system_.verify(verification_request).verified) {
        return failure(request.request_id, "The delete action ran, but verification did not pass.");
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "delete";
    result.result_data["entity_type"] = intent.entity_type;
    result.result_data["name"] = std::filesystem::path(path).filename().string();
    result.result_data["path"] = path;
    result.result_data["verified"] = "true";
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

    entity_resolution::ResolutionRequest file_request;
    file_request.query = intent.name;
    file_request.expected_type = entity_resolution::EntityType::File;
    const std::filesystem::path requested(intent.name);
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
    if (match.status == entity_resolution::ResolutionStatus::Ambiguous) {
        return needs_choice(request.request_id, "file");
    }
    if (match.status != entity_resolution::ResolutionStatus::Found) {
        return failure(request.request_id, "The text file could not be found.");
    }

    const auto path = match.selected.id;
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
    execution_request.operation = "read_text";
    execution_request.input_data = plan.tasks.front().input_data;
    execution_request.requires_authorization = true;
    execution_request.authorization_granted = true;
    const auto executed = execution_engine_.execute(execution_request);
    if (executed.status != execution::ExecutionStatus::Success) {
        plan_state_manager_.fail(plan, plan.tasks.front().id);
        return failure(request.request_id, executed.error);
    }
    plan_state_manager_.complete(plan, plan.tasks.front().id);

    const auto text_value = executed.output_data.find("text");
    if (text_value == executed.output_data.end()) {
        return failure(request.request_id, "The file reader returned no text content.");
    }
    if (text_value->second.find('\\0') != std::string::npos) {
        return failure(request.request_id, "The selected file appears to contain binary data.");
    }

    information::ProviderRegistry registry;
    information::InformationProcessor processor(registry);
    information::DataRequest data_request;
    data_request.operation = operation;
    data_request.text_input = text_value->second;
    const auto count = processor.process(data_request);
    if (!count.success) return failure(request.request_id, count.error);

    verification::VerificationRequest verification_request;
    verification_request.operation = "count_file_text";
    verification_request.expected["exists"] = "true";
    verification_request.actual["exists"] =
        platform_.filesystem().exists(path) ? "true" : "false";
    if (!verification_system_.verify(verification_request).verified) {
        return failure(request.request_id, "The file could not be verified after reading.");
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
    return result;
}


SubsystemResult CommandSubsystem::create_folder_chain(
    const SubsystemRequest& request, const std::vector<std::string>& clauses) {
    if (clauses.size() < 2) return handle_single(request, clauses.front());

    planning::PlanningRequest planning_request;
    planning_request.goal = "Execute a dependent folder-creation sequence";
    std::map<std::string, std::string> local_paths;
    std::vector<std::string> target_paths;
    std::vector<std::string> folder_names;
    target_paths.reserve(clauses.size());
    folder_names.reserve(clauses.size());

    for (std::size_t index = 0; index < clauses.size(); ++index) {
        const auto intent = grammar_engine_.interpret(clauses[index]);
        if (!intent.valid || intent.operation != "create" ||
            intent.entity_type != "folder") {
            return failure(request.request_id,
                "Multi-step v0.1 currently supports dependent folder-creation sequences only.");
        }
        const std::filesystem::path folder_name(intent.name);
        if (folder_name.empty() || folder_name.is_absolute() ||
            folder_name.has_parent_path() || intent.name == "." || intent.name == "..") {
            return failure(request.request_id, "A folder name must be a single path component.");
        }
        if (intent.destination.empty()) {
            return failure(request.request_id, "Every folder-creation step needs a destination.");
        }
        if (local_paths.contains(lower(intent.name))) {
            return failure(request.request_id,
                "A folder name is repeated in the same sequence; use distinct names.");
        }

        std::string destination;
        bool planned_destination = false;
        const auto local = local_paths.find(lower(intent.destination));
        if (local != local_paths.end()) {
            destination = local->second;
            planned_destination = true;
        } else {
            destination = resolve_destination(intent.destination);
        }

        std::error_code ec;
        if (destination.empty() ||
            (!planned_destination &&
             (!std::filesystem::is_directory(destination, ec) || ec))) {
            return failure(request.request_id,
                "A folder destination does not exist or could not be resolved.");
        }

        entity_resolution::ResolutionRequest resolution_request;
        resolution_request.query = std::filesystem::path(destination).filename().string();
        resolution_request.expected_type = entity_resolution::EntityType::Folder;
        resolution_request.context_hint = std::filesystem::path(destination).parent_path().string();
        resolution_request.candidates.push_back({
            destination, resolution_request.query,
            entity_resolution::EntityType::Folder, resolution_request.context_hint,
            "linux.filesystem", true, 0
        });
        const auto resolved = entity_resolver_.resolve(std::move(resolution_request));
        if (resolved.status != entity_resolution::ResolutionStatus::Found) {
            return failure(request.request_id, "A folder destination could not be resolved uniquely.");
        }

        const auto target = (std::filesystem::path(resolved.selected.id) / folder_name)
            .lexically_normal().string();

        reasoning::Fact fact;
        fact.subject = target;
        fact.property = "exists";
        fact.value = platform_.filesystem().exists(target) ? "true" : "false";
        fact.source = "linux.filesystem";
        fact.confidence = 100;
        fact.fresh = true;
        reasoning::ReasoningRequest reasoning_request;
        reasoning_request.query = {target, "exists", "false", true};
        reasoning_request.facts.push_back(std::move(fact));
        const auto reasoning_result = reasoning_engine_.evaluate(reasoning_request);
        if (reasoning_result.truth != TruthValue::True) {
            return failure(request.request_id,
                "A target folder already exists or its state could not be determined.");
        }

        permissions::PermissionRequest permission_request;
        permission_request.capability_id = "filesystem.create_directory";
        permission_request.operation = "create_directory";
        permission_request.resource_path = target;
        const auto permission = permission_manager_.authorize(permission_request);
        if (permission.decision != permissions::PermissionDecision::Allowed) {
            return failure(request.request_id, permission.reason);
        }

        planning::TaskDraft draft;
        draft.id = "folder-step-" + std::to_string(index + 1);
        draft.operation = "create_directory";
        draft.input_data["path"] = target;
        if (index > 0) draft.dependencies.push_back("folder-step-" + std::to_string(index));
        planning_request.tasks.push_back(std::move(draft));

        local_paths[lower(intent.name)] = target;
        target_paths.push_back(target);
        folder_names.push_back(intent.name);
    }

    auto plan = planning_engine_.create_plan(planning_request);
    if (!plan.valid || plan.tasks.size() != clauses.size()) {
        return failure(request.request_id, "The multi-step folder plan could not be validated.");
    }

    std::size_t completed = 0;
    for (std::size_t index = 0; index < plan.tasks.size(); ++index) {
        auto& task = plan.tasks[index];
        if (!plan_state_manager_.start(plan, task.id)) {
            return failure(request.request_id, "A dependent folder task could not start.");
        }

        execution::ExecutionRequest execution_request;
        execution_request.task_id = request.task_id + "-chain-" + std::to_string(index + 1);
        execution_request.operation = task.operation;
        execution_request.input_data = task.input_data;
        execution_request.requires_authorization = true;
        execution_request.authorization_granted = true;
        const auto executed = execution_engine_.execute(execution_request);
        if (executed.status != execution::ExecutionStatus::Success) {
            plan_state_manager_.fail(plan, task.id);
            auto result = failure(request.request_id, executed.error);
            result.result_data["completed_steps"] = std::to_string(completed);
            result.result_data["total_steps"] = std::to_string(clauses.size());
            return result;
        }

        verification::VerificationRequest verification_request;
        verification_request.operation = "create_directory";
        verification_request.expected["exists"] = "true";
        verification_request.actual["exists"] =
            platform_.filesystem().exists(target_paths[index]) ? "true" : "false";
        if (!verification_system_.verify(verification_request).verified) {
            plan_state_manager_.fail(plan, task.id);
            auto result = failure(request.request_id,
                "A folder was created but its verification did not pass.");
            result.result_data["completed_steps"] = std::to_string(completed);
            result.result_data["total_steps"] = std::to_string(clauses.size());
            return result;
        }

        plan_state_manager_.complete(plan, task.id);
        remember_folder(folder_names[index], target_paths[index]);
        ++completed;
    }

    SubsystemResult result;
    result.request_id = request.request_id;
    result.status = SubsystemStatus::Success;
    result.result_data["operation"] = "create_chain";
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

    const auto intent = grammar_engine_.interpret(raw_input);
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
    const auto clauses = split_instruction_chain(raw->second);
    if (clauses.size() > 1) return create_folder_chain(request, clauses);
    return handle_single(request, raw->second);
}

} // namespace seventh
