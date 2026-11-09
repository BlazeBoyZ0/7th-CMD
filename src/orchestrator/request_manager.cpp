#include "seventh/orchestrator/request_manager.hpp"

#include "seventh/core/identifiers.hpp"

#include <utility>

namespace seventh {

SubsystemRequest RequestManager::create(
    const std::string& operation,
    std::map<std::string, std::string> input_data,
    std::map<std::string, std::string> context,
    StringList constraints) const {
    SubsystemRequest request;
    request.request_id = make_identifier("request");
    request.task_id = make_identifier("task");
    request.operation = operation;
    request.input_data = std::move(input_data);
    request.context = std::move(context);
    request.constraints = std::move(constraints);
    return request;
}

} // namespace seventh
