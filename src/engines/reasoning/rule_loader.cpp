#include "seventh/engines/reasoning/rule_loader.hpp"

#include "seventh/utilities/json_loader.hpp"

#include <cmath>
#include <set>
#include <utility>

namespace seventh::reasoning {
namespace {

const utilities::JsonValue* member(
    const utilities::JsonValue::Object& object, const std::string& key) {
    const auto found = object.find(key);
    return found == object.end() ? nullptr : &found->second;
}

bool read_string(const utilities::JsonValue::Object& object,
                 const std::string& key, std::string& output,
                 bool required, std::string& error) {
    const auto* value = member(object, key);
    if (!value) {
        if (!required) return true;
        error = "Missing required string field '" + key + "'.";
        return false;
    }
    const auto* text = value->as_string();
    if (!text || (required && text->empty())) {
        error = "Field '" + key + "' must be a non-empty string.";
        return false;
    }
    output = *text;
    return true;
}

bool parse_operator(const std::string& value, ConditionOperator& operation) {
    if (value == "equal") operation = ConditionOperator::Equal;
    else if (value == "not_equal") operation = ConditionOperator::NotEqual;
    else if (value == "exists") operation = ConditionOperator::Exists;
    else return false;
    return true;
}

} // namespace

bool RuleLoader::load_file(const std::string& path,
                           std::vector<Rule>& rules,
                           std::string& error) const {
    utilities::JsonLoader loader;
    const auto parsed = loader.load_file(path, error);
    if (!parsed) return false;

    const auto* root = parsed->as_object();
    if (!root) {
        error = "The rule configuration root must be an object.";
        return false;
    }

    const auto* version_value = member(*root, "schema_version");
    const auto* version = version_value ? version_value->as_number() : nullptr;
    if (!version || *version != 1.0) {
        error = "The rule configuration has an unsupported schema version.";
        return false;
    }

    const auto* rules_value = member(*root, "rules");
    const auto* rule_array = rules_value ? rules_value->as_array() : nullptr;
    if (!rule_array) {
        error = "The rule configuration must contain a rules array.";
        return false;
    }

    std::vector<Rule> loaded;
    std::set<std::string> identifiers;
    for (std::size_t rule_index = 0; rule_index < rule_array->size(); ++rule_index) {
        const auto* object = (*rule_array)[rule_index].as_object();
        if (!object) {
            error = "Rule entry " + std::to_string(rule_index) +
                    " must be an object.";
            return false;
        }

        Rule rule;
        if (!read_string(*object, "id", rule.id, true, error)) return false;
        if (!identifiers.insert(rule.id).second) {
            error = "The rule configuration contains a duplicate rule ID.";
            return false;
        }

        const auto* premises_value = member(*object, "premises");
        const auto* premises = premises_value ? premises_value->as_array() : nullptr;
        if (!premises || premises->empty()) {
            error = "Rule '" + rule.id + "' must contain at least one premise.";
            return false;
        }

        for (std::size_t premise_index = 0;
             premise_index < premises->size(); ++premise_index) {
            const auto* premise = (*premises)[premise_index].as_object();
            if (!premise) {
                error = "A premise in rule '" + rule.id + "' must be an object.";
                return false;
            }

            RuleCondition condition;
            std::string operation;
            if (!read_string(*premise, "subject", condition.subject, true, error) ||
                !read_string(*premise, "property", condition.property, true, error) ||
                !read_string(*premise, "operator", operation, true, error)) {
                return false;
            }
            if (!parse_operator(operation, condition.operation)) {
                error = "Rule '" + rule.id + "' uses an unsupported condition operator.";
                return false;
            }
            if (!read_string(*premise, "value", condition.value,
                             condition.operation != ConditionOperator::Exists, error)) {
                return false;
            }
            rule.premises.push_back(std::move(condition));
        }

        const auto* conclusion_value = member(*object, "conclusion");
        const auto* conclusion = conclusion_value
            ? conclusion_value->as_object() : nullptr;
        if (!conclusion) {
            error = "Rule '" + rule.id + "' must contain a conclusion object.";
            return false;
        }

        if (!read_string(*conclusion, "subject", rule.conclusion.subject, true, error) ||
            !read_string(*conclusion, "property", rule.conclusion.property, true, error) ||
            !read_string(*conclusion, "value", rule.conclusion.value, true, error) ||
            !read_string(*conclusion, "source", rule.conclusion.source, false, error)) {
            return false;
        }

        if (const auto* confidence_value = member(*conclusion, "confidence")) {
            const auto* confidence = confidence_value->as_number();
            if (!confidence || !std::isfinite(*confidence) ||
                *confidence < 0.0 || *confidence > 100.0 ||
                std::floor(*confidence) != *confidence) {
                error = "Rule '" + rule.id + "' confidence must be an integer from 0 to 100.";
                return false;
            }
            rule.conclusion.confidence = static_cast<unsigned>(*confidence);
        }

        if (const auto* fresh_value = member(*conclusion, "fresh")) {
            const auto* fresh = fresh_value->as_bool();
            if (!fresh) {
                error = "Rule '" + rule.id + "' fresh must be a boolean.";
                return false;
            }
            rule.conclusion.fresh = *fresh;
        }

        loaded.push_back(std::move(rule));
    }

    rules = std::move(loaded);
    error.clear();
    return true;
}

} // namespace seventh::reasoning
