#pragma once

#include <string>
#include <vector>

namespace seventh::reasoning {

class ReasoningTrace {
public:
    void add(std::string event);
    [[nodiscard]] const std::vector<std::string>& events() const noexcept;
    void clear() noexcept;

private:
    std::vector<std::string> events_;
};

} // namespace seventh::reasoning
