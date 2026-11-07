#include "seventh/orchestrator/command_subsystem.hpp"

#include "seventh/platform/linux/linux_operation_executor.hpp"
#include "seventh/systems/entity_resolution/entity_types.hpp"
#include "seventh/systems/permissions/permission_types.hpp"
#include "seventh/systems/verification/verification_types.hpp"

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
    return (std::filesystem::path(home) / requested).lexically_normal().string();
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
    return result;
}

SubsystemResult CommandSubsystem::open_target(
    const SubsystemRequest& request, const grammar::CanonicalIntent& intent) {
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

    auto app_match = entity_resolver_.resolve(app_request);
    std::optional<platform::ApplicationEntry> selected_app;
    if (app_match.status == entity_resolution::ResolutionStatus::Found) {
        selected_app = platform_.applications().find(app_match.selected.id);
    } else if (lower(intent.name) == "browser") {
        selected_app = platform_.applications().find_default("browser");
        if (selected_app) {
            app_request.query = selected_app->name;
            app_request.candidates = {{
                selected_app->id, selected_app->name,
                entity_resolution::EntityType::Application,
                selected_app->desktop_file, "xdg.default", true, 0
            }};
            app_match = entity_resolver_.resolve(app_request);
        }
    }

    std::string operation;
    std::map<std::string, std::string> operation_data;
    std::string capability_id;
    std::string entity_type;
    std::string resolved_name;

    if (selected_app && app_match.status == entity_resolution::ResolutionStatus::Found) {
        operation = "launch_application";
        operation_data["application_id"] = selected_app->id;
        capability_id = "application.launch";
        entity_type = "application";
        resolved_name = selected_app->name;
    } else {
        if (app_match.status == entity_resolution::ResolutionStatus::Ambiguous) {
            return needs_choice(request.request_id, "application");
        }

        const char* home = std::getenv("HOME");
        if (!home) return failure(request.request_id, "The user's home directory is unavailable.");
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
                if (!entry.directory) continue;
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
        if (folder_match.status != entity_resolution::ResolutionStatus::Found) {
            return failure(request.request_id,
                lower(intent.name) == "browser"
                    ? "No default browser is configured on this system."
                    : "No installed application or matching folder was found.");
        }

        operation = "open_path";
        operation_data["path"] = folder_match.selected.id;
        capability_id = "filesystem.open_path";
        entity_type = "folder";
        resolved_name = folder_match.selected.name;
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

SubsystemResult CommandSubsystem::handle(const SubsystemRequest& request) {
    const auto raw = request.input_data.find("raw_input");
    if (raw == request.input_data.end() || raw->second.empty()) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = SubsystemStatus::NeedsInformation;
        result.missing_information.emplace_back("instruction text");
        return result;
    }

    const auto intent = grammar_engine_.interpret(raw->second);
    if (!intent.valid) {
        SubsystemResult result;
        result.request_id = request.request_id;
        result.status = intent.missing_information.empty()
            ? SubsystemStatus::Failure : SubsystemStatus::NeedsInformation;
        result.missing_information = intent.missing_information;
        result.error = intent.error;
        return result;
    }

    if (intent.operation == "open" && intent.entity_type == "application") {
        return open_target(request, intent);
    }
    if (intent.operation == "create" && intent.entity_type == "folder") {
        return create_folder(request, intent);
    }
    return failure(request.request_id,
        "The current end-to-end workflow supports folder creation and opening applications or folders.");
}

} // namespace seventh
