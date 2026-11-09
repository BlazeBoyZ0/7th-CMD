#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace seventh::platform {

struct PlatformResult {
    bool success{false};
    std::string error;
};

struct FileEntry {
    std::string name;
    std::string path;
    bool directory{false};
    std::uintmax_t size{0};
};

struct ApplicationEntry {
    std::string id;
    std::string name;
    std::string executable;
    std::string desktop_file;
};

struct ProcessEntry {
    std::uint64_t id{0};
    std::string name;
    std::string executable;
};

struct SystemInformation {
    std::string operating_system;
    std::string architecture;
    std::uint64_t total_memory_bytes{0};
    std::uint64_t available_memory_bytes{0};
};

} // namespace seventh::platform
