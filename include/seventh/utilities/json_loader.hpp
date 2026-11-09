#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace seventh::utilities {

class JsonValue {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;

    JsonValue();
    explicit JsonValue(std::nullptr_t);
    explicit JsonValue(bool value);
    explicit JsonValue(double value);
    explicit JsonValue(std::string value);
    explicit JsonValue(Array value);
    explicit JsonValue(Object value);

    [[nodiscard]] bool is_null() const noexcept;
    [[nodiscard]] const bool* as_bool() const noexcept;
    [[nodiscard]] const double* as_number() const noexcept;
    [[nodiscard]] const std::string* as_string() const noexcept;
    [[nodiscard]] const Array* as_array() const noexcept;
    [[nodiscard]] const Object* as_object() const noexcept;

private:
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value_;
};

class JsonLoader {
public:
    [[nodiscard]] std::optional<JsonValue> parse(
        const std::string& input, std::string& error) const;
    [[nodiscard]] std::optional<JsonValue> load_file(
        const std::string& path, std::string& error) const;
};

} // namespace seventh::utilities
