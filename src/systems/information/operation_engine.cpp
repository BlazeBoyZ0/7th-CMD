#include "seventh/systems/information/operation_engine.hpp"

#include "seventh/systems/information/type_system.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace seventh::information {
namespace {

std::string lower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string format_number(double value) {
    if (value == 0.0) return "0";
    std::ostringstream output;
    output << std::setprecision(15) << value;
    return output.str();
}

std::optional<std::vector<double>> collect_numbers(
    const std::vector<DataRecord>& records, const std::string& field,
    std::string& error) {
    if (field.empty()) {
        error = "A field is required for numeric aggregation.";
        return std::nullopt;
    }
    if (records.empty()) {
        error = "There are no records to aggregate.";
        return std::nullopt;
    }

    std::vector<double> values;
    values.reserve(records.size());
    for (const auto& record : records) {
        const auto found = record.find(field);
        if (found == record.end()) {
            error = "A record is missing numeric field '" + field + "'.";
            return std::nullopt;
        }
        const auto number = TypeSystem::number(found->second);
        if (!number) {
            error = "Field '" + field + "' contains a non-numeric value.";
            return std::nullopt;
        }
        values.push_back(*number);
    }
    return values;
}

std::size_t utf8_codepoint_count(const std::string& text) {
    std::size_t count = 0;
    for (const unsigned char byte : text) {
        if ((byte & 0xC0U) != 0x80U) ++count;
    }
    return count;
}

std::vector<std::string> split_words(const std::string& text) {
    std::istringstream input(text);
    std::vector<std::string> words;
    std::string word;
    while (input >> word) words.push_back(std::move(word));
    return words;
}

} // namespace

DataResult OperationEngine::execute(const DataRequest& request,
                                    const std::vector<DataRecord>& records) const {
    DataResult result;
    result.records = records;
    const auto& operation = request.operation;

    if (operation == "count") {
        result.scalar = std::to_string(records.size());

    } else if (operation == "find" || operation == "filter") {
        if (request.field.empty()) {
            result.error = "A field is required for find/filter.";
            return result;
        }
        result.records.clear();
        for (const auto& record : records) {
            const auto field = record.find(request.field);
            if (field != record.end() &&
                lower(field->second).find(lower(request.value)) != std::string::npos) {
                result.records.push_back(record);
            }
        }
        result.scalar = std::to_string(result.records.size());

    } else if (operation == "sort") {
        if (request.field.empty()) {
            result.error = "A field is required for sort.";
            return result;
        }
        std::stable_sort(result.records.begin(), result.records.end(),
            [&](const DataRecord& left, const DataRecord& right) {
                const auto a = left.find(request.field);
                const auto b = right.find(request.field);
                const bool a_missing = a == left.end();
                const bool b_missing = b == right.end();
                if (a_missing != b_missing) return !a_missing;
                if (a_missing) return false;

                const auto an = TypeSystem::number(a->second);
                const auto bn = TypeSystem::number(b->second);
                const bool less = an && bn ? *an < *bn : a->second < b->second;
                const bool equal = an && bn ? *an == *bn : a->second == b->second;
                return request.descending ? (!less && !equal) : less;
            });

    } else if (operation == "group") {
        if (request.field.empty()) {
            result.error = "A field is required for group.";
            return result;
        }
        std::map<std::string, std::size_t> groups;
        for (const auto& record : records) {
            const auto found = record.find(request.field);
            ++groups[found == record.end() ? std::string{} : found->second];
        }
        result.records.clear();
        for (const auto& [value, count] : groups) {
            result.records.push_back({{"value", value}, {"count", std::to_string(count)}});
        }
        result.scalar = std::to_string(groups.size());

    } else if (operation == "sum" || operation == "average" ||
               operation == "min" || operation == "max") {
        std::string error;
        const auto values = collect_numbers(records, request.field, error);
        if (!values) {
            result.error = std::move(error);
            return result;
        }

        double aggregate = 0.0;
        if (operation == "sum" || operation == "average") {
            for (const double value : *values) aggregate += value;
            if (operation == "average") {
                aggregate /= static_cast<double>(values->size());
            }
        } else if (operation == "min") {
            aggregate = *std::min_element(values->begin(), values->end());
        } else {
            aggregate = *std::max_element(values->begin(), values->end());
        }
        result.scalar = format_number(aggregate);

    } else if (operation == "add" || operation == "subtract") {
        const auto left = TypeSystem::number(request.value);
        const auto right = TypeSystem::number(request.second_value);
        if (!left || !right) {
            result.error = "add/subtract requires two finite numeric values.";
            return result;
        }
        const double value = operation == "add" ? *left + *right : *left - *right;
        if (!std::isfinite(value)) {
            result.error = "The numeric result is outside the supported range.";
            return result;
        }
        result.scalar = format_number(value);

    } else if (operation == "compare" || operation == "compare_values") {
        const auto left = TypeSystem::number(request.value);
        const auto right = TypeSystem::number(request.second_value);
        if (left && right) {
            result.scalar = *left < *right ? "less"
                : (*left > *right ? "greater" : "equal");
        } else {
            result.scalar = request.value < request.second_value ? "less"
                : (request.value > request.second_value ? "greater" : "equal");
        }

    } else if (operation == "get_property") {
        if (request.field.empty()) {
            result.error = "A field is required for get_property.";
            return result;
        }
        if (records.empty()) {
            result.error = "There are no records to read.";
            return result;
        }
        const auto found = records.front().find(request.field);
        if (found == records.front().end()) {
            result.error = "The requested field does not exist.";
            return result;
        }
        result.scalar = found->second;

    } else if (operation == "select_fields") {
        if (request.fields.empty()) {
            result.error = "At least one field is required for select_fields.";
            return result;
        }
        result.records.clear();
        for (const auto& record : records) {
            DataRecord selected;
            for (const auto& field : request.fields) {
                const auto found = record.find(field);
                if (found != record.end()) selected.emplace(found->first, found->second);
            }
            result.records.push_back(std::move(selected));
        }
        result.scalar = std::to_string(result.records.size());

    } else if (operation == "compare_records") {
        if (records.size() < 2) {
            result.error = "compare_records requires at least two records.";
            return result;
        }

        std::set<std::string> fields(request.fields.begin(), request.fields.end());
        if (fields.empty()) {
            for (const auto& [key, value] : records[0]) {
                (void)value;
                fields.insert(key);
            }
            for (const auto& [key, value] : records[1]) {
                (void)value;
                fields.insert(key);
            }
        }

        result.records.clear();
        bool all_equal = true;
        for (const auto& field : fields) {
            const auto left = records[0].find(field);
            const auto right = records[1].find(field);
            const std::string left_value = left == records[0].end() ? std::string{} : left->second;
            const std::string right_value = right == records[1].end() ? std::string{} : right->second;
            const bool equal = left != records[0].end() &&
                               right != records[1].end() &&
                               left_value == right_value;
            all_equal = all_equal && equal;
            result.records.push_back({
                {"field", field},
                {"left", left_value},
                {"right", right_value},
                {"equal", equal ? "true" : "false"}
            });
        }
        result.scalar = all_equal ? "equal" : "different";

    } else if (operation == "word_count") {
        result.scalar = std::to_string(split_words(request.text_input).size());

    } else if (operation == "tokenize") {
        result.records.clear();
        const auto words = split_words(request.text_input);
        for (std::size_t index = 0; index < words.size(); ++index) {
            result.records.push_back({
                {"index", std::to_string(index)},
                {"token", words[index]}
            });
        }
        result.scalar = std::to_string(words.size());

    } else if (operation == "character_count") {
        result.scalar = std::to_string(utf8_codepoint_count(request.text_input));

    } else if (operation == "line_count") {
        if (request.text_input.empty()) {
            result.scalar = "0";
        } else {
            const auto newlines = static_cast<std::size_t>(
                std::count(request.text_input.begin(), request.text_input.end(), '\n'));
            result.scalar = std::to_string(
                newlines + (request.text_input.back() == '\n' ? 0U : 1U));
        }

    } else if (operation == "text_search" || operation == "search") {
        result.scalar = lower(request.text_input).find(lower(request.value)) ==
            std::string::npos ? "false" : "true";

    } else if (operation == "extract") {
        if (request.value.empty()) {
            result.error = "A search value is required for extract.";
            return result;
        }
        result.records.clear();
        std::istringstream lines(request.text_input);
        std::string line;
        std::size_t line_number = 0;
        while (std::getline(lines, line)) {
            ++line_number;
            if (lower(line).find(lower(request.value)) != std::string::npos) {
                result.records.push_back({
                    {"line_number", std::to_string(line_number)},
                    {"text", line}
                });
            }
        }
        result.scalar = std::to_string(result.records.size());

    } else {
        result.error = "Unsupported information operation: " + operation;
        return result;
    }

    result.success = true;
    return result;
}

} // namespace seventh::information
