#include "seventh/engines/grammar/grammar_engine.hpp"
#include "seventh/engines/grammar/input_processor.hpp"
#include "seventh/engines/grammar/typo_corrector.hpp"
#include "seventh/engines/grammar/phrase_composer.hpp"
#include "seventh/cli/terminal_output.hpp"
#include "seventh/platform/linux/linux_filesystem.hpp"
#include "seventh/engines/planning/planning_engine.hpp"
#include "seventh/engines/reasoning/rule_evaluator.hpp"
#include "seventh/engines/reasoning/truth_evaluator.hpp"
#include "seventh/systems/permissions/permission_manager.hpp"
#include "seventh/systems/information/platform_data_providers.hpp"
#include "seventh/orchestrator/task_state_manager.hpp"
#include "seventh/orchestrator/workflow_controller.hpp"
#include "seventh/systems/entity_resolution/entity_resolver.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <unistd.h>

namespace {

class TestFilesystem final : public seventh::platform::FilesystemInterface {
public:
    std::string content;

    bool exists(const std::string&) const override { return false; }
    std::vector<seventh::platform::FileEntry> list(
        const std::string&, std::string&) const override { return {}; }
    std::optional<std::string> read_text(
        const std::string&, std::string&) const override { return content; }
    seventh::platform::PlatformResult create_directory(
        const std::string&) override { return {}; }
    seventh::platform::PlatformResult rename(
        const std::string&, const std::string&) override { return {}; }
    seventh::platform::PlatformResult copy(
        const std::string&, const std::string&) override { return {}; }
    seventh::platform::PlatformResult remove(
        const std::string&) override { return {}; }
};

class TestApplications final : public seventh::platform::ApplicationInterface {
public:
    std::vector<seventh::platform::ApplicationEntry> list(
        std::string&) const override { return {}; }
    std::optional<seventh::platform::ApplicationEntry> find(
        const std::string&) const override { return std::nullopt; }
    std::optional<seventh::platform::ApplicationEntry> find_default(
        const std::string&) const override { return std::nullopt; }
    seventh::platform::PlatformResult launch(
        const seventh::platform::ApplicationEntry&) override { return {}; }
};

class TestProcesses final : public seventh::platform::ProcessInterface {
public:
    std::vector<seventh::platform::ProcessEntry> list(
        std::string&) const override { return {}; }
    seventh::platform::PlatformResult terminate(std::uint64_t) override { return {}; }
};

class TestSystem final : public seventh::platform::SystemInterface {
public:
    seventh::platform::SystemInformation information(
        std::string&) const override { return {}; }
};

class TestPlatform final : public seventh::platform::PlatformInterface {
public:
    const char* id() const noexcept override { return "test"; }
    seventh::platform::FilesystemInterface& filesystem() noexcept override {
        return filesystem_;
    }
    seventh::platform::ApplicationInterface& applications() noexcept override {
        return applications_;
    }
    seventh::platform::ProcessInterface& processes() noexcept override {
        return processes_;
    }
    seventh::platform::SystemInterface& system() noexcept override {
        return system_;
    }

    TestFilesystem filesystem_;
    TestApplications applications_;
    TestProcesses processes_;
    TestSystem system_;
};

int failures = 0;

void expect(bool condition, const std::string& description) {
    if (condition) {
        std::cout << "[PASS] " << description << '\n';
    } else {
        std::cerr << "[FAIL] " << description << '\n';
        ++failures;
    }
}


void test_document_provider_rejects_binary_content() {
    using namespace seventh::information;

    TestPlatform platform;
    ProviderRegistry registry;
    register_platform_data_providers(registry, platform);

    DataRequest request;
    request.provider_id = "document";
    request.source_path = "sample.txt";
    const auto provider = registry.find(request);
    expect(static_cast<bool>(provider),
           "the document data provider is registered");

    if (!provider) return;

    std::vector<DataRecord> records;
    std::string error;
    platform.filesystem_.content = "ordinary text";
    expect(provider->acquire(request, records, error),
           "the document provider accepts ordinary text content");
    expect(!records.empty() && records.front().at("text") == "ordinary text",
           "the document provider returns the accepted text");

    platform.filesystem_.content = std::string("before\0after", 12);
    error.clear();
    expect(!provider->acquire(request, records, error) &&
               error.find("binary") != std::string::npos,
           "the document provider rejects embedded null bytes as binary content");
}

void test_grammar_input_metadata() {
    using namespace seventh::grammar;

    const Normalizer normalizer;
    const auto normalized = normalizer.normalize(
        "  Create\t a folder named \"My  Games\"\r\n on Desktop...  ");
    expect(normalized == "Create a folder named \"My  Games\" on Desktop...",
           "normalization collapses external whitespace but preserves quoted values");

    const InputProcessor processor;
    const auto tokens = processor.process_detailed(
        "Create a folder named \"My  Games\" on Desktop...");
    expect(tokens.size() == 7,
           "detailed tokenization keeps quoted multi-word values as one token");
    if (tokens.size() == 7) {
        expect(tokens[4].text == "My  Games" && tokens[4].quoted,
               "quoted token content and quote metadata are preserved");
        expect(tokens[0].begin == 0 && tokens[0].end == 6 &&
                   tokens[0].normalized == "create",
               "tokens retain byte offsets and a lowercase matching form");
        expect(tokens.back().punctuation_after == "...",
               "trailing punctuation is recorded without becoming a token");
    }

    const auto filename = processor.process_detailed("open main.cpp");
    expect(filename.size() == 2 && filename[1].text == "main.cpp",
           "periods inside filenames are not treated as sentence punctuation");

    const auto clauses = processor.process_detailed(
        "make a folder, name it Games; put it on Desktop!");
    expect(clauses.size() == 10 &&
               clauses[2].punctuation_after == "," &&
               clauses[5].punctuation_after == ";",
           "clause punctuation is retained as token metadata");
}


void test_grammar_typo_correction() {
    using namespace seventh::grammar;

    const TypoCorrector corrector;
    const std::set<std::string> vocabulary{
        "create", "folder", "directory", "application", "remove", "search"
    };
    const std::map<std::string, std::string> explicit_corrections{
        {"creat", "create"}, {"foler", "folder"}
    };

    Token token;
    token.text = "cretae";
    auto result = corrector.correct(token, vocabulary, explicit_corrections, true);
    expect(result.changed && result.value == "create" && result.distance == 1,
           "the typo corrector recognizes an adjacent transposition");

    token.text = "direcotry";
    result = corrector.correct(token, vocabulary, explicit_corrections, true);
    expect(result.changed && result.value == "directory" && result.distance == 2,
           "long vocabulary terms allow a bounded two-edit correction");

    token.text = "creat";
    result = corrector.correct(token, vocabulary, explicit_corrections, true);
    expect(result.changed && result.value == "create",
           "explicit spelling corrections take priority over fuzzy matching");

    token.text = "foler";
    result = corrector.correct(token, vocabulary, explicit_corrections, true);
    expect(result.changed && result.value == "folder",
           "known entity spelling corrections remain supported");

    token.text = "Games";
    result = corrector.correct(token, vocabulary, explicit_corrections, true);
    expect(!result.changed && result.value == "Games",
           "unmatched user terms retain their original spelling and case");

    token.text = "cretae";
    token.quoted = true;
    result = corrector.correct(token, vocabulary, explicit_corrections, true);
    expect(!result.changed && result.value == "cretae",
           "quoted values are never spelling-corrected");

    token.quoted = false;
    result = corrector.correct(token, vocabulary, explicit_corrections, false);
    expect(!result.changed && result.value == "cretae",
           "tokens outside an explicitly marked grammar position are untouched");

    token.text = "cot";
    const std::set<std::string> ambiguous_vocabulary{"cat", "cut"};
    result = corrector.correct(token, ambiguous_vocabulary, {}, true);
    expect(!result.changed && result.ambiguous && result.value == "cot",
           "equally close vocabulary candidates are reported as ambiguous");

    expect(TypoCorrector::damerau_levenshtein("form", "from") == 1,
           "the edit-distance metric counts adjacent transposition as one edit");
}


void test_grammar_phrase_composition() {
    using namespace seventh::grammar;
    const GrammarEngine grammar;

    const auto punctuation = grammar.interpret(
        "make a folder, name it Games; put it on Desktop");
    expect(punctuation.valid && punctuation.operation == "create" &&
               punctuation.entity_type == "folder" &&
               punctuation.name == "Games" &&
               punctuation.destination == "Desktop",
           "separate name and destination phrases compose into one folder intent");

    const auto conjunctions = grammar.interpret(
        "make a folder and name it Projects and place it on Desktop");
    expect(conjunctions.valid && conjunctions.name == "Projects" &&
               conjunctions.destination == "Desktop",
           "and-connected follow-up phrases compose into one folder intent");

    const auto then_phrases = grammar.interpret(
        "create a folder then call it BuildArea then put it in Documents");
    expect(then_phrases.valid && then_phrases.name == "BuildArea" &&
               then_phrases.destination == "Documents",
           "then-separated naming and location phrases are composed");

    const auto quoted_name = grammar.interpret(
        "make a folder, name it \"My, Games\"; put it on Desktop");
    expect(quoted_name.valid && quoted_name.name == "My, Games" &&
               quoted_name.destination == "Desktop",
           "quoted names remain intact while surrounding phrases are composed");

    const auto conversational = grammar.interpret(
        "hey dude, create a folder named Batch1Test on Desktop, okay?");
    expect(conversational.valid && conversational.name == "Batch1Test" &&
               conversational.destination == "Desktop",
           "recognized courtesy preambles and closings do not obscure a folder command");

    const auto typo_and_casual = grammar.interpret(
        "hey dude, make new diraectory, name it Games; place it on Desktop, okay?");
    expect(typo_and_casual.valid && typo_and_casual.entity_type == "folder" &&
               typo_and_casual.name == "Games" &&
               typo_and_casual.destination == "Desktop",
           "courtesy words, a known entity typo, and follow-up phrases compose safely");

    const auto original_style = grammar.interpret(
        "hey dude, make new diraectory.. name itt Games.. and place it on Desktop.. okay?");
    expect(original_style.valid && original_style.operation == "create" &&
               original_style.entity_type == "folder" &&
               original_style.name == "Games" &&
               original_style.destination == "Desktop",
           "casual multi-clause wording, a directory typo, and a pronoun typo compose correctly");

    const auto ordinary = grammar.interpret(
        "create a folder named Existing on Desktop");
    expect(ordinary.valid && ordinary.name == "Existing" &&
               ordinary.destination == "Desktop",
           "ordinary single-clause folder commands retain their existing behavior");

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

    const auto mixed_case_question = grammar.interpret("how muxh words are in Test.Kek");
    expect(mixed_case_question.valid && mixed_case_question.name == "Test.Kek",
           "question normalization preserves the case of file names");

    const auto typo_command = grammar.interpret("creat a foler named Scratch on Desktop");
    expect(typo_command.valid && typo_command.operation == "create" &&
               typo_command.entity_type == "folder" &&
               typo_command.name == "Scratch" && typo_command.destination == "Desktop",
           "known command and entity typos are corrected while names and paths are preserved");

    const auto incomplete = grammar.interpret("create a folder");
    expect(!incomplete.valid && !incomplete.missing_information.empty(),
           "incomplete instructions request missing information");

    expect(create.schema_version == 1 && !create.id.empty() &&
               create.slots.contains("name") && create.slots.contains("destination"),
           "CIR carries a schema version, unique identifier, and structured slots");
    expect(create.pattern_id == "create_named_entity",
           "the declarative create pattern is selected and recorded");
    const auto repeated_create = grammar.interpret("Create a folder named Projects on Desktop");
    expect(!repeated_create.id.empty() && repeated_create.id != create.id,
           "separate interpretations receive distinct CIR identifiers");

    seventh::grammar::GrammarConfig loaded_config;
    std::string grammar_error;
    const bool config_loaded = seventh::grammar::GrammarConfigLoader{}.load_directory(
        SEVENTH_CMD_TEST_GRAMMAR_DIR, loaded_config, grammar_error);
    expect(config_loaded && !loaded_config.patterns.empty(),
           "the versioned JSON grammar patterns load into executable definitions");
    if (config_loaded) {
        loaded_config.patterns.front().target_markers = {"called"};
        loaded_config.patterns.front().location_markers = {"at"};
        const auto configurable = GrammarEngine(loaded_config).interpret(
            "create a folder called DynamicPatternTest at Desktop");
        expect(configurable.valid && configurable.name == "DynamicPatternTest" &&
                   configurable.destination == "Desktop",
               "changing JSON-backed pattern markers changes actual parsing behavior");
    }

    const auto partial = grammar.interpret("rename file main.cpp");
    expect(!partial.valid && partial.missing_information.size() == 1 &&
               partial.missing_information.front() == "destination",
           "partial parsing preserves the source slot and requests only the missing destination");

    const auto nested_named = grammar.interpret("create a new folder inside it named Minecraft");
    expect(nested_named.valid && nested_named.name == "Minecraft" &&
               nested_named.destination == "it",
           "slot extraction supports a destination before the named target");
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



bool contains_error(const seventh::StringList& errors, const std::string& fragment) {
    for (const auto& error : errors) {
        if (error.find(fragment) != std::string::npos) return true;
    }
    return false;
}

void test_planning_stable_order_and_dependency_normalization() {
    using namespace seventh::planning;
    PlanningRequest request;
    request.goal = "Build a dependency-safe task sequence";

    TaskDraft child;
    child.id = "child";
    child.operation = "create_directory";
    child.dependencies = {"parent", "parent"};
    TaskDraft first;
    first.id = "first";
    first.operation = "open_path";
    TaskDraft parent;
    parent.id = "parent";
    parent.operation = "create_directory";
    TaskDraft second;
    second.id = "second";
    second.operation = "read_text";

    request.tasks = {child, first, parent, second};
    const auto plan = PlanningEngine{}.create_plan(request);
    expect(plan.valid && plan.tasks.size() == 4,
           "a valid multi-step plan is accepted after dependency normalization");
    if (plan.tasks.size() == 4) {
        expect(plan.tasks[0].id == "first" && plan.tasks[1].id == "parent" &&
                   plan.tasks[2].id == "child" && plan.tasks[3].id == "second",
               "topological ordering is deterministic and stable among ready tasks");
        expect(plan.tasks[2].dependencies.size() == 1,
               "duplicate dependency declarations are normalized");
    }
}

void test_planning_rejects_malformed_dependencies() {
    using namespace seventh::planning;
    PlanningRequest request;
    request.goal = "Validate malformed dependencies";
    TaskDraft task;
    task.id = "task";
    task.operation = "create_directory";
    task.dependencies = {""};
    request.tasks = {task};

    const auto empty = PlanningEngine{}.create_plan(request);
    expect(!empty.valid && contains_error(empty.errors, "empty dependency"),
           "an empty dependency is reported instead of silently discarded");

    request.tasks[0].dependencies = {"task"};
    const auto self = PlanningEngine{}.create_plan(request);
    expect(!self.valid && contains_error(self.errors, "depend on itself"),
           "a task cannot depend on itself");

    request.tasks[0].dependencies = {"missing"};
    const auto missing = PlanningEngine{}.create_plan(request);
    expect(!missing.valid && contains_error(missing.errors, "Missing dependency"),
           "a dependency on an absent task is rejected");
}

void test_planning_rejects_duplicate_ids_and_cycles_without_reordering() {
    using namespace seventh::planning;
    PlanningRequest request;
    request.goal = "Reject ambiguous task graphs";
    TaskDraft first;
    first.id = "same";
    first.operation = "create_directory";
    TaskDraft second = first;
    request.tasks = {first, second};

    const auto duplicate = PlanningEngine{}.create_plan(request);
    expect(!duplicate.valid &&
               contains_error(duplicate.errors, "Duplicate task identifier"),
           "duplicate task identifiers are rejected before scheduling");

    request.tasks[0].id = "first";
    request.tasks[0].dependencies = {"second"};
    request.tasks[1].id = "second";
    request.tasks[1].dependencies = {"first"};
    const auto cyclic = PlanningEngine{}.create_plan(request);
    expect(!cyclic.valid && contains_error(cyclic.errors, "cycle"),
           "a cyclic graph is rejected");
    expect(cyclic.tasks.size() == 2 && cyclic.tasks[0].id == "first" &&
               cyclic.tasks[1].id == "second",
           "a failed schedule leaves the original task order intact");
}

void test_planning_limits_and_required_fields() {
    using namespace seventh::planning;
    PlanningRequest request;
    request.goal = "Bound planner resource use";
    request.max_tasks = 1;
    TaskDraft first;
    first.id = "first";
    first.operation = "create_directory";
    TaskDraft second;
    second.id = "second";
    second.operation = "create_directory";
    request.tasks = {first, second};

    const auto too_many_tasks = PlanningEngine{}.create_plan(request);
    expect(!too_many_tasks.valid && too_many_tasks.tasks.empty() &&
               contains_error(too_many_tasks.errors, "task limit"),
           "the task limit is enforced before copying the task list");

    request.max_tasks = 8;
    request.max_dependency_edges = 0;
    request.tasks[1].dependencies = {"first"};
    const auto too_many_edges = PlanningEngine{}.create_plan(request);
    expect(!too_many_edges.valid &&
               contains_error(too_many_edges.errors, "dependency limit"),
           "the dependency-edge limit is enforced before normalization");

    request.max_dependency_edges = 8;
    request.tasks.resize(1);
    request.tasks[0].operation = "   ";
    const auto blank_operation = PlanningEngine{}.create_plan(request);
    expect(!blank_operation.valid &&
               contains_error(blank_operation.errors, "no operation"),
           "whitespace-only operations are rejected");

    request.tasks[0].operation = "create_directory";
    request.goal = "  ";
    const auto blank_goal = PlanningEngine{}.create_plan(request);
    expect(!blank_goal.valid && contains_error(blank_goal.errors, "no goal"),
           "whitespace-only plan goals are rejected");
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

void test_terminal_output_escaping() {
    const std::string hostile = std::string("name") + char(0x1B) +
        "[31m" + "\xE2\x80\xAE" + "txt";
    const auto escaped = seventh::cli::escape_terminal_text(hostile);
    expect(escaped == "name\\x1B[31m\\u{202E}txt",
           "terminal control and bidi formatting characters are escaped");
    expect(seventh::cli::escape_terminal_text("caf\xC3\xA9") == "caf\xC3\xA9",
           "ordinary UTF-8 filename characters are preserved");
    expect(seventh::cli::escape_terminal_text(std::string("bad") + char(0xFF)) ==
               "bad\\xFF",
           "invalid UTF-8 bytes are escaped rather than passed to the terminal");
}

void test_descriptor_anchored_filesystem_operations() {
    using namespace seventh::capabilities;
    using namespace seventh::permissions;
    using seventh::platform::linux_backend::LinuxFilesystem;

    char temporary[] = "/tmp/7thcmd-secure-XXXXXX";
    const char* created = ::mkdtemp(temporary);
    expect(created != nullptr, "a temporary filesystem security fixture is created");
    if (!created) return;

    const std::filesystem::path base(created);
    const auto root = base / "root";
    const auto outside = base / "outside";
    std::filesystem::create_directory(root);
    std::filesystem::create_directory(outside);
    const auto link = root / "escape";
    const bool linked = ::symlink(outside.c_str(), link.c_str()) == 0;
    expect(linked, "the filesystem security fixture creates an outside-pointing symlink");

    CapabilityRegistry registry;
    CapabilityDefinition capability;
    capability.id = "test.secure-write";
    capability.operations = {"create_directory", "rename_path", "delete_path"};
    capability.allowed_roots = {root.string()};
    capability.enabled = true;
    capability.read_only = false;
    expect(registry.register_capability(capability),
           "a descriptor-scoped test capability is registered");

    PermissionManager permissions(registry);
    PermissionRequest request;
    request.capability_id = capability.id;
    request.operation = "create_directory";
    request.resource_path = (root / "inside" / "child").string();
    request.modifies_state = true;
    const auto authorized = permissions.authorize(request);
    expect(authorized.decision == PermissionDecision::Allowed &&
               static_cast<bool>(authorized.authorized_root),
           "authorization returns a live directory-handle capability");

    LinuxFilesystem filesystem;
    if (authorized.authorized_root) {
        const auto created_folder = filesystem.create_directory_authorized(
            request.resource_path, *authorized.authorized_root);
        expect(created_folder.success,
               "descriptor-relative creation safely creates missing parent directories");
        expect(!filesystem.create_directory((root / "legacy-bypass").string()).success,
               "legacy path-only mutation APIs fail closed without an authorized handle");

        const auto escaped_target = (link / "outside-write").string();
        request.resource_path = escaped_target;
        const auto symlink_authorized = permissions.authorize(request);
        expect(symlink_authorized.decision == PermissionDecision::Allowed,
               "lexical authorization alone does not pretend to resolve a symlink");
        if (symlink_authorized.authorized_root) {
            const auto blocked = filesystem.create_directory_authorized(
                escaped_target, *symlink_authorized.authorized_root);
            expect(!blocked.success &&
                       !std::filesystem::exists(outside / "outside-write"),
                   "descriptor traversal refuses a symlink escape outside the authorized root");
        } else {
            expect(false, "the symlink test obtains its root handle");
        }

        const auto source = (root / "inside" / "child").string();
        const auto destination = (root / "inside" / "renamed").string();
        const auto renamed = filesystem.rename_authorized(
            source, destination, *authorized.authorized_root,
            *authorized.authorized_root);
        expect(renamed.success && std::filesystem::is_directory(destination),
               "descriptor-relative rename moves a directory without following path links");

        const auto removed = filesystem.remove_authorized(
            destination, *authorized.authorized_root);
        expect(removed.success && !std::filesystem::exists(destination),
               "descriptor-relative removal deletes the authorized target");
        const auto removed_link = filesystem.remove_authorized(
            link.string(), *authorized.authorized_root);
        expect(removed_link.success && !std::filesystem::exists(link) &&
                   std::filesystem::exists(outside),
               "descriptor-relative removal unlinks a symlink without touching its target");
    }

    std::error_code ignored;
    std::filesystem::remove(link, ignored);
    std::filesystem::remove_all(base, ignored);
}

} // namespace

int main() {
    test_document_provider_rejects_binary_content();
    test_grammar_input_metadata();
    test_grammar_typo_correction();
    test_grammar_phrase_composition();
    test_grammar_interpretation();
    test_entity_resolution_relevance();
    test_three_valued_logic();
    test_rule_inference();
    test_planning_dependencies();
    test_planning_stable_order_and_dependency_normalization();
    test_planning_rejects_malformed_dependencies();
    test_planning_rejects_duplicate_ids_and_cycles_without_reordering();
    test_planning_limits_and_required_fields();
    test_permission_boundaries();
    test_task_lifecycle();
    test_terminal_output_escaping();
    test_descriptor_anchored_filesystem_operations();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All grammar, engine, and permission tests passed.\n";
    return EXIT_SUCCESS;
}
