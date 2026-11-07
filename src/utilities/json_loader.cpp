#include "seventh/utilities/json_loader.hpp"

#include "seventh/core/constants.hpp"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <utility>

namespace seventh::utilities {
namespace {

class Parser {
public:
    explicit Parser(const std::string& input) : input_(input) {}

    std::optional<JsonValue> parse(std::string& error) {
        skip_space();
        auto value = parse_value(0);
        if (!value) {
            error = error_;
            return std::nullopt;
        }
        skip_space();
        if (position_ != input_.size()) {
            fail("Unexpected characters after the JSON value.");
            error = error_;
            return std::nullopt;
        }
        return value;
    }

private:
    const std::string& input_;
    std::size_t position_{0};
    std::string error_;

    void skip_space() {
        while (position_ < input_.size() &&
               (input_[position_] == ' ' || input_[position_] == '\t' ||
                input_[position_] == '\r' || input_[position_] == '\n')) ++position_;
    }

    void fail(const std::string& message) {
        if (error_.empty()) error_ = message + " At byte " + std::to_string(position_) + ".";
    }

    bool consume(char expected) {
        if (position_ >= input_.size() || input_[position_] != expected) return false;
        ++position_;
        return true;
    }

    std::optional<JsonValue> parse_value(std::size_t depth) {
        if (depth > 64) {
            fail("JSON nesting limit exceeded.");
            return std::nullopt;
        }
        skip_space();
        if (position_ >= input_.size()) {
            fail("Unexpected end of JSON input.");
            return std::nullopt;
        }

        const char ch = input_[position_];
        if (ch == '"') {
            auto value = parse_string();
            if (!value) return std::nullopt;
            return JsonValue(std::move(*value));
        }
        if (ch == '{') return parse_object(depth + 1);
        if (ch == '[') return parse_array(depth + 1);
        if (ch == 't' && literal("true")) return JsonValue(true);
        if (ch == 'f' && literal("false")) return JsonValue(false);
        if (ch == 'n' && literal("null")) return JsonValue(nullptr);
        if (ch == '-' || (ch >= '0' && ch <= '9')) return parse_number();

        fail("Unexpected token in JSON input.");
        return std::nullopt;
    }

    bool literal(const char* word) {
        std::size_t offset = 0;
        while (word[offset] != '\0') {
            if (position_ + offset >= input_.size() ||
                input_[position_ + offset] != word[offset]) return false;
            ++offset;
        }
        position_ += offset;
        return true;
    }

    static void append_utf8(std::string& output, unsigned int codepoint) {
        if (codepoint <= 0x7F) output.push_back(static_cast<char>(codepoint));
        else if (codepoint <= 0x7FF) {
            output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    std::optional<std::string> parse_string() {
        if (!consume('"')) {
            fail("Expected a JSON string.");
            return std::nullopt;
        }
        std::string output;
        while (position_ < input_.size()) {
            const unsigned char ch = static_cast<unsigned char>(input_[position_++]);
            if (ch == '"') return output;
            if (ch < 0x20) {
                fail("Unescaped control character in JSON string.");
                return std::nullopt;
            }
            if (ch != '\\') {
                output.push_back(static_cast<char>(ch));
                continue;
            }
            if (position_ >= input_.size()) break;
            const char escaped = input_[position_++];
            switch (escaped) {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case '/': output.push_back('/'); break;
            case 'b': output.push_back('\b'); break;
            case 'f': output.push_back('\f'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            case 'u': {
                if (position_ + 4 > input_.size()) {
                    fail("Incomplete Unicode escape.");
                    return std::nullopt;
                }
                unsigned int codepoint = 0;
                for (int i = 0; i < 4; ++i) {
                    const char digit = input_[position_++];
                    codepoint <<= 4;
                    if (digit >= '0' && digit <= '9') codepoint |= digit - '0';
                    else if (digit >= 'a' && digit <= 'f') codepoint |= digit - 'a' + 10;
                    else if (digit >= 'A' && digit <= 'F') codepoint |= digit - 'A' + 10;
                    else {
                        fail("Invalid Unicode escape.");
                        return std::nullopt;
                    }
                }
                if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
                    fail("Unicode surrogate escapes are not supported in this version.");
                    return std::nullopt;
                }
                append_utf8(output, codepoint);
                break;
            }
            default:
                fail("Invalid escape sequence in JSON string.");
                return std::nullopt;
            }
        }
        fail("Unterminated JSON string.");
        return std::nullopt;
    }

    std::optional<JsonValue> parse_number() {
        const auto start = position_;
        consume('-');
        if (consume('0')) {
            if (position_ < input_.size() && input_[position_] >= '0' &&
                input_[position_] <= '9') {
                fail("Leading zero in JSON number.");
                return std::nullopt;
            }
        } else {
            const auto digits = position_;
            while (position_ < input_.size() &&
                   input_[position_] >= '0' && input_[position_] <= '9') ++position_;
            if (digits == position_) {
                fail("Invalid JSON number.");
                return std::nullopt;
            }
        }
        if (consume('.')) {
            const auto digits = position_;
            while (position_ < input_.size() &&
                   input_[position_] >= '0' && input_[position_] <= '9') ++position_;
            if (digits == position_) {
                fail("Missing digits after decimal point.");
                return std::nullopt;
            }
        }
        if (position_ < input_.size() &&
            (input_[position_] == 'e' || input_[position_] == 'E')) {
            ++position_;
            if (position_ < input_.size() &&
                (input_[position_] == '+' || input_[position_] == '-')) ++position_;
            const auto digits = position_;
            while (position_ < input_.size() &&
                   input_[position_] >= '0' && input_[position_] <= '9') ++position_;
            if (digits == position_) {
                fail("Missing exponent digits.");
                return std::nullopt;
            }
        }
        try {
            return JsonValue(std::stod(input_.substr(start, position_ - start)));
        } catch (...) {
            fail("JSON number is outside the supported range.");
            return std::nullopt;
        }
    }

    std::optional<JsonValue> parse_array(std::size_t depth) {
        consume('[');
        JsonValue::Array values;
        skip_space();
        if (consume(']')) return JsonValue(std::move(values));
        while (true) {
            auto value = parse_value(depth);
            if (!value) return std::nullopt;
            values.push_back(std::move(*value));
            skip_space();
            if (consume(']')) break;
            if (!consume(',')) {
                fail("Expected ',' or ']' in JSON array.");
                return std::nullopt;
            }
        }
        return JsonValue(std::move(values));
    }

    std::optional<JsonValue> parse_object(std::size_t depth) {
        consume('{');
        JsonValue::Object values;
        skip_space();
        if (consume('}')) return JsonValue(std::move(values));
        while (true) {
            skip_space();
            auto key = parse_string();
            if (!key) return std::nullopt;
            skip_space();
            if (!consume(':')) {
                fail("Expected ':' after JSON object key.");
                return std::nullopt;
            }
            auto value = parse_value(depth);
            if (!value) return std::nullopt;
            if (!values.emplace(std::move(*key), std::move(*value)).second) {
                fail("Duplicate JSON object key.");
                return std::nullopt;
            }
            skip_space();
            if (consume('}')) break;
            if (!consume(',')) {
                fail("Expected ',' or '}' in JSON object.");
                return std::nullopt;
            }
        }
        return JsonValue(std::move(values));
    }
};

} // namespace

JsonValue::JsonValue() : value_(nullptr) {}
JsonValue::JsonValue(std::nullptr_t) : value_(nullptr) {}
JsonValue::JsonValue(bool value) : value_(value) {}
JsonValue::JsonValue(double value) : value_(value) {}
JsonValue::JsonValue(std::string value) : value_(std::move(value)) {}
JsonValue::JsonValue(Array value) : value_(std::move(value)) {}
JsonValue::JsonValue(Object value) : value_(std::move(value)) {}

bool JsonValue::is_null() const noexcept {
    return std::holds_alternative<std::nullptr_t>(value_);
}
const bool* JsonValue::as_bool() const noexcept { return std::get_if<bool>(&value_); }
const double* JsonValue::as_number() const noexcept { return std::get_if<double>(&value_); }
const std::string* JsonValue::as_string() const noexcept {
    return std::get_if<std::string>(&value_);
}
const JsonValue::Array* JsonValue::as_array() const noexcept {
    return std::get_if<Array>(&value_);
}
const JsonValue::Object* JsonValue::as_object() const noexcept {
    return std::get_if<Object>(&value_);
}

std::optional<JsonValue> JsonLoader::parse(
    const std::string& input, std::string& error) const {
    return Parser(input).parse(error);
}

std::optional<JsonValue> JsonLoader::load_file(
    const std::string& path, std::string& error) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Unable to open JSON file: " + path;
        return std::nullopt;
    }
    input.seekg(0, std::ios::end);
    const auto length = input.tellg();
    if (length < 0 ||
        static_cast<unsigned long long>(length) > constants::max_json_config_bytes) {
        error = "JSON file exceeds the configured size limit.";
        return std::nullopt;
    }
    input.seekg(0, std::ios::beg);
    std::string contents{std::istreambuf_iterator<char>(input),
                         std::istreambuf_iterator<char>()};
    return parse(contents, error);
}

} // namespace seventh::utilities
