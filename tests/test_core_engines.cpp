#include "seventh/engines/grammar/grammar_engine.hpp"
#include "seventh/engines/planning/planning_engine.hpp"
#include "seventh/engines/reasoning/rule_evaluator.hpp"
#include "seventh/engines/reasoning/truth_evaluator.hpp"
#include "seventh/systems/permissions/permission_manager.hpp"
#include "seventh/orchestrator/task_state_manager.hpp"
#include "seventh/orchestrator/workflow_controller.hpp"
#include "seventh/systems/entity_resolution/entity_resolver.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const std::string& description) {
    if (condition) {
        std::cout << "[PASS] " << description << '\n';
    } else {
        std::cerr << "[FAIL] " << description << '\n';
        ++failures;
    }
}

void test_grammar_interpretation() {
    using seventh::grammar::GrammarEngine;
    const GrammarEngine grammar;

    const auto create = grammar.interpret("Create a folder named Projects on Desktop");
    expect(create.valid, "the grammar accepts a folder creation instruction");
    expect(create.operation == "create" && create.entity_type == "folder",
           "the grammar identifies the create-folder operation");
    expect(create.name == "Projects" && create.destination == "Desktop",
           "the grammar preserves folder name case and destination");

    const auto open = grammar.interpret("open browser");
    expect(open.valid && open.operation == "open" &&
               open.entity_type == "application" && open.name == "browser",
           "the grammar recognizes the default-browser instruction");

    const auto rename = grammar.interpret("rename file main.cpp to test.cpp");
    expect(rename.valid && rename.operation == "rename" &&
               rename.source == "main.cpp" && rename.destination == "test.cpp",
           "the grammar extracts rename source and destination");

    const auto search = grammar.interpret("search files named report in Documents");
    expect(search.valid && search.operation == "search" &&
               search.query == "report" && search.destination == "Documents",
           "the grammar extracts a search query and location");

    const auto nested = grammar.interpret("create a new folder inside it named Minecraft");
    expect(nested.valid && nested.name == "Minecraft" && nested.destination == "it",
           "the grammar extracts a folder name following its inside destination");

    const auto scoped_search = grammar.interpret("search for folders named Projects on Desktop");
    expect(scoped_search.valid && scoped_search.query == "Projects" &&
               scoped_search.destination == "Desktop",
           "the grammar extracts an on-Desktop search scope");

    const auto count_command = grammar.interpret("count lnes in test.kek");
    expect(count_command.valid && count_command.operation == "count" &&
               count_command.query == "lines" && count_command.name == "test.kek",
           "count commands normalize metric typos without changing file names");

    const auto count_question = grammar.interpret("how muxh words are in test.kek");
    expect(count_question.valid && count_question.operation == "count" &&
               count_question.query == "words" && count_question.name == "test.kek",
           "natural count questions normalize into a file-count instruction");

    const auto typo_command = grammar.interpret("creat a foler named Scratch on Desktop");
    expect(typo_command.valid && typo_command.operation == "create" &&
               typo_command.entity_type == "folder" &&
               typo_command.name == "Scratch" && typo_command.destination == "Desktop",
           "known command and entity typos are corrected while names and paths are preserved");

    const auto incomplete = grammar.interpret("create a folder");
    expect(!incomplete.valid && !incomplete.missing_information.empty(),
           "incomplete instructions request missing information");
}

void test_entity_resolution_relevance() {
    using namespace seventh::entity_resolution;
    ResolutionRequest unrelated;
    unrelated.query = "Alpha";
    unrelated.expected_type = EntityType::Application;
    unrelated.context_hint = "/home/test";
    unrelated.candidates.push_back({
        "about.desktop", "About Me", EntityType::Application,
        "/home/test/about.desktop", "test", true, 0
    });
    const auto no_match = EntityResolver{}.resolve(unrelated);
    expect(no_match.status == ResolutionStatus::NotFound &&
               no_match.candidates.empty(),
           "unrelated same-type candidates do not become false matches");

    ResolutionRequest duplicates;
    duplicates.query = "test.kek";
    duplicates.expected_type = EntityType::File;
    duplicates.candidates.push_back({
        "/home/test/Desktop/test.kek", "test.kek", EntityType::File,
        "/home/test/Desktop", "test", true, 0
    });
    duplicates.candidates.push_back({
        "/home/test/Documents/test.kek", "test.kek", EntityType::File,
        "/home/test/Documents", "test", true, 0
    });
    const auto ambiguous = EntityResolver{}.resolve(duplicates);
    expect(ambiguous.status == ResolutionStatus::Ambiguous &&
               ambiguous.candidates.size() == 2,
           "genuine duplicate exact matches remain available for clarification");

    ResolutionRequest weak_match;
    weak_match.query = "testfolder";
    weak_match.expected_type = EntityType::Folder;
    weak_match.candidates.push_back({
        "/home/test/test", "test", EntityType::Folder,
        "/home/test", "test", true, 0
    });
    const auto low_confidence = EntityResolver{}.resolve(weak_match);
    expect(low_confidence.status == ResolutionStatus::Ambiguous &&
               low_confidence.candidates.size() == 1,
           "a weak unique partial match requires user confirmation");
}

void test_three_valued_logic() {
    using seventh::TruthValue;
    using seventh::reasoning::TruthEvaluator;

    expect(TruthEvaluator::negate(TruthValue::Unknown) == TruthValue::Unknown,
           "negating UNKNOWN preserves UNKNOWN");
    expect(TruthEvaluator::conjunction(TruthValue::False, TruthValue::Unknown) ==
               TruthValue::False,
           "FALSE AND UNKNOWN is FALSE");
    expect(TruthEvaluator::conjunction(TruthValue::True, TruthValue::Unknown) ==
               TruthValue::Unknown,
           "TRUE AND UNKNOWN is UNKNOWN");
    expect(TruthEvaluator::disjunction(TruthValue::True, TruthValue::Unknown) ==
               TruthValue::True,
           "TRUE OR UNKNOWN is TRUE");
    expect(TruthEvaluator::disjunction(TruthValue::False, TruthValue::Unknown) ==
               TruthValue::Unknown,
           "FALSE OR UNKNOWN is UNKNOWN");
}

void test_rule_inference() {
    using namespace seventh::reasoning;

    Fact known;
    known.subject = "desktop";
    known.property = "exists";
    known.value = "true";
    known.source = "test";
    known.confidence = 95;

    Rule rule;
    rule.id = "desktop-is-available";
    rule.premises.push_back(
        {"desktop", "exists", "true", ConditionOperator::Equal});
    rule.conclusion.subject = "desktop";
    rule.conclusion.property = "available";
    rule.conclusion.value = "true";
    rule.conclusion.confidence = 90;

    const auto result = RuleEvaluator{}.evaluate({rule}, {known}, 8);
    expect(result.derived.size() == 1,
           "a satisfied rule derives its conclusion");
    expect(!result.budget_exhausted,
           "a small rule set completes within its evaluation budget");
    if (!result.derived.empty()) {
        expect(result.derived.front().value == "true",
               "the derived fact preserves the rule conclusion");
        expect(result.derived.front().confidence == 90,
               "derived confidence cannot exceed premise confidence");
    }

    known.fresh = false;
    const auto stale_result = RuleEvaluator{}.evaluate({rule}, {known}, 8);
    expect(stale_result.derived.empty(),
           "stale facts cannot satisfy rule premises");
}

void test_planning_dependencies() {
    using namespace seventh::planning;

    PlanningRequest request;
    request.goal = "Create a parent and then a child folder";

    TaskDraft child;
    child.id = "child";
    child.operation = "create_directory";
    child.dependencies = {"parent"};

    TaskDraft parent;
    parent.id = "parent";
    parent.operation = "create_directory";

    request.tasks = {child, parent};
    const auto plan = PlanningEngine{}.create_plan(request);

    expect(plan.valid, "a plan with valid dependencies is accepted");
    expect(plan.tasks.size() == 2, "both planned tasks are retained");
    if (plan.tasks.size() == 2) {
        expect(plan.tasks[0].id == "parent" && plan.tasks[1].id == "child",
               "tasks are ordered after their prerequisites");
    }

    request.tasks[0].dependencies = {"missing"};
    const auto missing = PlanningEngine{}.create_plan(request);
    expect(!missing.valid, "a plan with a missing dependency is rejected");

    request.tasks[0].dependencies = {"parent"};
    request.tasks[1].dependencies = {"child"};
    const auto cyclic = PlanningEngine{}.create_plan(request);
    expect(!cyclic.valid, "a cyclic dependency graph is rejected");
}


void test_task_lifecycle() {
    using namespace seventh;

    TaskStateManager states;
    expect(states.create("task-1"), "a new task can be registered");
    expect(!states.create("task-1"), "duplicate task identifiers are rejected");
    expect(!states.transition("task-1", TaskState::Completed),
           "a task cannot skip directly from NEW to COMPLETED");
    expect(states.transition("task-1", TaskState::Understanding),
           "NEW can transition to UNDERSTANDING");
    expect(states.transition("task-1", TaskState::Routing),
           "UNDERSTANDING can transition to ROUTING");
    expect(states.transition("task-1", TaskState::Processing),
           "ROUTING can transition to PROCESSING");
    expect(states.transition("task-1", TaskState::WaitingForUser),
           "PROCESSING can pause for clarification");
    expect(states.transition("task-1", TaskState::Processing),
           "a clarified task can resume processing");
    expect(states.transition("task-1", TaskState::Completed),
           "a processed task can complete");
    expect(!states.transition("task-1", TaskState::Processing),
           "a completed task cannot be restarted");

    WorkflowController workflow;
    SubsystemResult result;
    result.status = SubsystemStatus::NeedsPermission;
    expect(workflow.next_state(result) == TaskState::WaitingForPermission,
           "permission requests pause the task in the permission state");
    result.status = SubsystemStatus::NeedsInformation;
    expect(workflow.next_state(result) == TaskState::WaitingForUser,
           "missing information pauses the task for the user");
    result.status = SubsystemStatus::Failure;
    expect(workflow.next_state(result) == TaskState::Failed,
           "subsystem failures move the task to FAILED");
}

void test_permission_boundaries() {
    using namespace seventh::capabilities;
    using namespace seventh::permissions;

    CapabilityRegistry registry;
    const auto root = std::filesystem::current_path().lexically_normal().string();

    CapabilityDefinition read;
    read.id = "test.read";
    read.operations = {"read_text"};
    read.allowed_roots = {root};
    read.enabled = true;
    read.read_only = true;
    expect(registry.register_capability(read),
           "a valid read-only capability can be registered");

    CapabilityDefinition write;
    write.id = "test.delete";
    write.operations = {"delete_path"};
    write.allowed_roots = {root};
    write.enabled = true;
    write.read_only = false;
    expect(registry.register_capability(write),
           "a valid modifying capability can be registered");

    PermissionManager permissions(registry);

    PermissionRequest read_request;
    read_request.capability_id = "test.read";
    read_request.operation = "read_text";
    read_request.resource_path = root;
    expect(permissions.authorize(read_request).decision == PermissionDecision::Allowed,
           "an enabled read-only operation is allowed inside its root");

    read_request.modifies_state = true;
    expect(permissions.authorize(read_request).decision == PermissionDecision::Denied,
           "a read-only capability cannot authorize state changes");

    PermissionRequest delete_request;
    delete_request.capability_id = "test.delete";
    delete_request.operation = "delete_path";
    delete_request.resource_path = root;
    delete_request.destructive = true;
    expect(permissions.authorize(delete_request).decision ==
               PermissionDecision::RequiresConfirmation,
           "destructive operations require explicit confirmation");
    delete_request.user_confirmed = true;
    expect(permissions.authorize(delete_request).decision == PermissionDecision::Allowed,
           "a confirmed destructive operation can proceed");

    delete_request.resource_path = (std::filesystem::path(root) / "..").string();
    expect(permissions.authorize(delete_request).decision == PermissionDecision::Denied,
           "a target outside the configured root is denied");

    registry.set_enabled("test.delete", false);
    delete_request.resource_path = root;
    expect(permissions.authorize(delete_request).decision == PermissionDecision::Denied,
           "disabled capabilities cannot authorize operations");
}

} // namespace

int main() {
    test_grammar_interpretation();
    test_entity_resolution_relevance();
    test_three_valued_logic();
    test_rule_inference();
    test_planning_dependencies();
    test_permission_boundaries();
    test_task_lifecycle();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All grammar, engine, and permission tests passed.\n";
    return EXIT_SUCCESS;
}
