#include "seventh/engines/planning/planning_engine.hpp"
#include "seventh/engines/reasoning/rule_evaluator.hpp"
#include "seventh/engines/reasoning/truth_evaluator.hpp"

#include <cstdlib>
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

    // Deliberately provide the dependent task first. The planner must order it.
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

} // namespace

int main() {
    test_three_valued_logic();
    test_rule_inference();
    test_planning_dependencies();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All core engine tests passed.\n";
    return EXIT_SUCCESS;
}
