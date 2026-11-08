#include "seventh/cli/terminal_output.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace seventh::cli {
namespace {

bool terminal_format_control(char32_t codepoint) {
    return codepoint <= 0x1F ||
           (codepoint >= 0x7F && codepoint <= 0x9F) ||
           codepoint == 0x00AD || codepoint == 0x061C ||
           codepoint == 0x200B || codepoint == 0x200E ||
           codepoint == 0x200F || codepoint == 0x2028 ||
           codepoint == 0x2029 ||
           (codepoint >= 0x202A && codepoint <= 0x202E) ||
           (codepoint >= 0x2060 && codepoint <= 0x2069) ||
           codepoint == 0xFEFF;
}

void append_byte_escape(std::string& output, unsigned char byte) {
    constexpr char digits[] = "0123456789ABCDEF";
    output += "\\x";
    output += digits[(byte >> 4) & 0x0F];
    output += digits[byte & 0x0F];
}

void append_codepoint_escape(std::string& output, char32_t codepoint) {
    std::ostringstream escaped;
    escaped << "\\u{" << std::uppercase << std::hex
            << static_cast<std::uint32_t>(codepoint) << '}';
    output += escaped.str();
}

} // namespace

std::string escape_terminal_text(std::string_view text) {
    std::string output;
    output.reserve(text.size());

    for (std::size_t index = 0; index < text.size();) {
        const auto first = static_cast<unsigned char>(text[index]);
        if (first < 0x80) {
            if (terminal_format_control(first)) append_byte_escape(output, first);
            else output.push_back(static_cast<char>(first));
            ++index;
            continue;
        }

        std::size_t length = 0;
        char32_t codepoint = 0;
        if ((first & 0xE0) == 0xC0) {
            length = 2; codepoint = first & 0x1F;
        } else if ((first & 0xF0) == 0xE0) {
            length = 3; codepoint = first & 0x0F;
        } else if ((first & 0xF8) == 0xF0) {
            length = 4; codepoint = first & 0x07;
        } else {
            append_byte_escape(output, first);
            ++index;
            continue;
        }

        if (index + length > text.size()) {
            append_byte_escape(output, first);
            ++index;
            continue;
        }

        bool valid = true;
        for (std::size_t offset = 1; offset < length; ++offset) {
            const auto next = static_cast<unsigned char>(text[index + offset]);
            if ((next & 0xC0) != 0x80) {
                valid = false;
                break;
            }
            codepoint = (codepoint << 6) | (next & 0x3F);
        }

        const bool overlong = (length == 2 && codepoint < 0x80) ||
                              (length == 3 && codepoint < 0x800) ||
                              (length == 4 && codepoint < 0x10000);
        if (!valid || overlong || codepoint > 0x10FFFF ||
            (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
            append_byte_escape(output, first);
            ++index;
            continue;
        }

        if (terminal_format_control(codepoint)) {
            append_codepoint_escape(output, codepoint);
        } else {
            output.append(text.substr(index, length));
        }
        index += length;
    }
    return output;
}

void print_startup_message() {
    std::cout
        << "7th cmd v0.1 foundation build.\n"
        << "Command mode is active; enter an instruction to begin.\n"
        << "Use --help to see available options.\n";
}

void print_message(const std::string_view message) {
    std::cout << escape_terminal_text(message) << '\n';
}

void print_error(const std::string_view message) {
    std::cerr << "Error: " << escape_terminal_text(message) << '\n';
}

} // namespace seventh::cli
