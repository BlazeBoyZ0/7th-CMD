#pragma once

#include <cstddef>

namespace seventh::constants {

inline constexpr std::size_t max_text_read_bytes = 4U * 1024U * 1024U;
inline constexpr std::size_t max_json_config_bytes = 1U * 1024U * 1024U;
inline constexpr std::size_t default_rule_evaluation_budget = 128;
inline constexpr std::size_t default_recovery_attempts = 2;

} // namespace seventh::constants
