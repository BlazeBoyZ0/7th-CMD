#include "seventh/systems/information/data_normalizer.hpp"

#include <cctype>

namespace seventh::information {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

} // namespace

void DataNormalizer::normalize(std::vector<DataRecord>& records) const {
    for (auto& record : records) {
        DataRecord normalized;
        for (auto& [key, value] : record) {
            normalized[trim(key)] = trim(value);
        }
        record = std::move(normalized);
    }
}

} // namespace seventh::information
