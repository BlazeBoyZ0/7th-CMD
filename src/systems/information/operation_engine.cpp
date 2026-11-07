#include "seventh/systems/information/operation_engine.hpp"

#include "seventh/systems/information/type_system.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace seventh::information {
namespace {

std::string lower(std::string value) {
    for (char& ch : value) ch = static_cast<char>(
        std::tolower(static_cast<unsigned char>(ch)));
    return value;
}

} // namespace

DataResult OperationEngine::execute(const DataRequest& request,
                                    const std::vector<DataRecord>& records) const {
    DataResult result;
    result.records = records;

    if (request.operation == "count") {
        result.scalar = std::to_string(records.size());
    } else if (request.operation == "find" || request.operation == "filter") {
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
    } else if (request.operation == "sort") {
        if (request.field.empty()) {
            result.error = "A field is required for sort.";
            return result;
        }
        std::stable_sort(result.records.begin(), result.records.end(),
            [&](const DataRecord& left, const DataRecord& right) {
                const auto a = left.find(request.field);
                const auto b = right.find(request.field);
                if (a == left.end()) return true;
                if (b == right.end()) return false;
                const auto an = TypeSystem::number(a->second);
                const auto bn = TypeSystem::number(b->second);
                if (an && bn) return *an < *bn;
                return a->second < b->second;
            });
    } else if (request.operation == "word_count") {
        std::istringstream words(request.text_input);
        std::string word;
        std::size_t count = 0;
        while (words >> word) ++count;
        result.scalar = std::to_string(count);
    } else if (request.operation == "character_count") {
        result.scalar = std::to_string(request.text_input.size());
    } else if (request.operation == "line_count") {
        if (request.text_input.empty()) result.scalar = "0";
        else result.scalar = std::to_string(
            1 + std::count(request.text_input.begin(), request.text_input.end(), '\n'));
    } else if (request.operation == "text_search") {
        result.scalar = request.text_input.find(request.value) == std::string::npos
            ? "false" : "true";
    } else {
        result.error = "Unsupported information operation: " + request.operation;
        return result;
    }

    result.success = true;
    return result;
}

} // namespace seventh::information
