#pragma once

#include <map>
#include <string>
#include <vector>

namespace seventh::information {

using DataRecord = std::map<std::string, std::string>;

struct DataRequest {
    std::string provider_id;
    std::string operation;
    std::string field;
    std::string value;
    std::string text_input;
};

struct DataResult {
    bool success{false};
    std::string scalar;
    std::string summary;
    std::vector<DataRecord> records;
    std::string error;
};

} // namespace seventh::information
