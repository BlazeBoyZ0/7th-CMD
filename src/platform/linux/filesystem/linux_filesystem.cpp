#include "seventh/platform/linux/linux_filesystem.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <utility>

namespace seventh::platform::linux_backend {
namespace fs = std::filesystem;
namespace {

PlatformResult failure(const std::string& message) {
    return {false, message};
}

bool usable_path(const std::string& path) {
    return !path.empty() && fs::path(path).is_absolute();
}

} // namespace

bool LinuxFilesystem::exists(const std::string& path) const {
    std::error_code error;
    return usable_path(path) && fs::exists(path, error) && !error;
}

std::vector<FileEntry> LinuxFilesystem::list(
    const std::string& directory, std::string& error) const {
    std::vector<FileEntry> entries;
    if (!usable_path(directory)) {
        error = "A non-empty absolute directory path is required.";
        return entries;
    }

    std::error_code ec;
    fs::directory_iterator iterator(directory, ec);
    if (ec) {
        error = ec.message();
        return entries;
    }
    const fs::directory_iterator end;
    while (iterator != end) {
        const auto& entry = *iterator;
        FileEntry item;
        item.name = entry.path().filename().string();
        item.path = entry.path().string();
        item.directory = entry.is_directory(ec);
        if (ec) ec.clear();
        if (!item.directory && entry.is_regular_file(ec)) {
            item.size = entry.file_size(ec);
            if (ec) {
                item.size = 0;
                ec.clear();
            }
        }
        entries.push_back(std::move(item));
        iterator.increment(ec);
        if (ec) {
            error = ec.message();
            break;
        }
    }
    return entries;
}

std::optional<std::string> LinuxFilesystem::read_text(
    const std::string& path, std::string& error) const {
    constexpr std::uintmax_t max_bytes = 4 * 1024 * 1024;
    if (!usable_path(path)) {
        error = "A non-empty absolute file path is required.";
        return std::nullopt;
    }

    std::error_code ec;
    if (!fs::is_regular_file(path, ec) || ec) {
        error = ec ? ec.message() : "The target is not a regular file.";
        return std::nullopt;
    }
    const auto size = fs::file_size(path, ec);
    if (ec || size > max_bytes) {
        error = ec ? ec.message() : "The file exceeds the 4 MiB text-read limit.";
        return std::nullopt;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Unable to open the file for reading.";
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

PlatformResult LinuxFilesystem::create_directory(const std::string& path) {
    if (!usable_path(path)) return failure("An absolute directory path is required.");
    std::error_code ec;
    if (fs::exists(path, ec)) {
        if (ec) return failure(ec.message());
        return failure("The destination already exists.");
    }
    if (ec) return failure(ec.message());

    const bool created = fs::create_directories(path, ec);
    if (ec) return failure(ec.message());
    if (!created) return failure("The directory was not created.");
    return {true, {}};
}

PlatformResult LinuxFilesystem::rename(
    const std::string& source, const std::string& destination) {
    if (!usable_path(source) || !usable_path(destination))
        return failure("Absolute source and destination paths are required.");
    std::error_code ec;
    if (fs::exists(destination, ec)) return failure("The destination already exists.");
    if (ec) return failure(ec.message());
    fs::rename(source, destination, ec);
    return ec ? failure(ec.message()) : PlatformResult{true, {}};
}

PlatformResult LinuxFilesystem::copy(
    const std::string& source, const std::string& destination) {
    if (!usable_path(source) || !usable_path(destination))
        return failure("Absolute source and destination paths are required.");
    std::error_code ec;
    const bool copied = fs::copy_file(source, destination, fs::copy_options::none, ec);
    if (ec) return failure(ec.message());
    return copied ? PlatformResult{true, {}} : failure("The file was not copied.");
}

PlatformResult LinuxFilesystem::remove(const std::string& path) {
    if (!usable_path(path)) return failure("An absolute path is required.");
    std::error_code ec;
    const bool removed = fs::remove(path, ec);
    if (ec) return failure(ec.message());
    return removed ? PlatformResult{true, {}} : failure("The target was not removed.");
}

} // namespace seventh::platform::linux_backend
