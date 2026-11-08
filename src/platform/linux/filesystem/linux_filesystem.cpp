#include "seventh/platform/linux/linux_filesystem.hpp"

#include "seventh/systems/permissions/permission_types.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace seventh::platform::linux_backend {
namespace fs = std::filesystem;
namespace {

PlatformResult failure(const std::string& message) {
    return {false, message};
}

std::string system_error(const char* action) {
    return std::string(action) + ": " + std::strerror(errno);
}

bool usable_path(const std::string& path) {
    return !path.empty() && fs::path(path).is_absolute();
}

struct ParentHandle {
    int descriptor{-1};
    std::string leaf;

    ~ParentHandle() {
        if (descriptor >= 0) ::close(descriptor);
    }
    ParentHandle() = default;
    ParentHandle(const ParentHandle&) = delete;
    ParentHandle& operator=(const ParentHandle&) = delete;
    ParentHandle(ParentHandle&& other) noexcept
        : descriptor(std::exchange(other.descriptor, -1)),
          leaf(std::move(other.leaf)) {}
};

bool relative_components(
    const std::string& path, const permissions::AuthorizedRoot& root,
    std::vector<std::string>& components, std::string& error) {
    if (root.descriptor < 0 || root.canonical_path.empty() || root.canonical_path.find(char(0)) != std::string::npos || path.find(char(0)) != std::string::npos || !usable_path(path)) {
        error = "A valid authorized directory handle and absolute target path are required.";
        return false;
    }

    const auto target = fs::path(path).lexically_normal();
    const auto root_path = fs::path(root.canonical_path);
    const auto relative = target.lexically_relative(root_path);
    if (relative.empty() || relative.is_absolute()) {
        error = "The target is outside the authorized directory.";
        return false;
    }

    for (const auto& component : relative) {
        const auto value = component.string();
        if (value.empty() || value == "." || value == ".." || value == "/") {
            error = "The target contains an unsafe path component.";
            return false;
        }
        components.push_back(value);
    }
    if (components.empty()) {
        error = "Operations on the authorized directory itself are not permitted.";
        return false;
    }
    return true;
}

ParentHandle open_parent(
    const std::vector<std::string>& components,
    const permissions::AuthorizedRoot& root,
    bool create_missing, std::string& error) {
    ParentHandle result;
    result.descriptor = ::fcntl(root.descriptor, F_DUPFD_CLOEXEC, 0);
    if (result.descriptor < 0) {
        error = system_error("Unable to duplicate the authorized directory handle");
        return result;
    }

    for (std::size_t index = 0; index + 1 < components.size(); ++index) {
        int next = ::openat(result.descriptor, components[index].c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (next < 0 && errno == ENOENT && create_missing) {
            if (::mkdirat(result.descriptor, components[index].c_str(), 0755) < 0 &&
                errno != EEXIST) {
                error = system_error("Unable to create a parent directory");
                return result;
            }
            next = ::openat(result.descriptor, components[index].c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        }
        if (next < 0) {
            error = system_error("Unable to safely open a parent directory");
            return result;
        }
        ::close(result.descriptor);
        result.descriptor = next;
    }

    result.leaf = components.back();
    return result;
}

bool get_components(
    const std::string& path, const permissions::AuthorizedRoot& root,
    std::vector<std::string>& components, std::string& error) {
    return relative_components(path, root, components, error);
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

PlatformResult LinuxFilesystem::create_directory(const std::string&) {
    return failure("Directory creation requires an authorized directory handle.");
}

PlatformResult LinuxFilesystem::rename(const std::string&, const std::string&) {
    return failure("Rename requires authorized source and destination directory handles.");
}

PlatformResult LinuxFilesystem::copy(const std::string&, const std::string&) {
    return failure("Copy requires authorized source and destination directory handles.");
}

PlatformResult LinuxFilesystem::remove(const std::string&) {
    return failure("Removal requires an authorized directory handle.");
}

PlatformResult LinuxFilesystem::create_directory_authorized(
    const std::string& path, const permissions::AuthorizedRoot& root) {
    std::vector<std::string> components;
    std::string error;
    if (!get_components(path, root, components, error)) return failure(error);

    int current = ::fcntl(root.descriptor, F_DUPFD_CLOEXEC, 0);
    if (current < 0) return failure(system_error("Unable to duplicate authorized root"));

    for (std::size_t index = 0; index < components.size(); ++index) {
        const bool final = index + 1 == components.size();
        if (final) {
            if (::mkdirat(current, components[index].c_str(), 0755) < 0) {
                error = system_error("Unable to create the requested directory");
                ::close(current);
                return failure(error);
            }
            ::close(current);
            return {true, {}};
        }

        int next = ::openat(current, components[index].c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (next < 0 && errno == ENOENT) {
            if (::mkdirat(current, components[index].c_str(), 0755) < 0 &&
                errno != EEXIST) {
                error = system_error("Unable to create a parent directory");
                ::close(current);
                return failure(error);
            }
            next = ::openat(current, components[index].c_str(),
                             O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        }
        if (next < 0) {
            error = system_error("Unable to safely open a parent directory");
            ::close(current);
            return failure(error);
        }
        ::close(current);
        current = next;
    }
    ::close(current);
    return failure("The requested directory was not created.");
}

PlatformResult LinuxFilesystem::rename_authorized(
    const std::string& source, const std::string& destination,
    const permissions::AuthorizedRoot& source_root,
    const permissions::AuthorizedRoot& destination_root) {
    std::vector<std::string> source_parts, destination_parts;
    std::string error;
    if (!get_components(source, source_root, source_parts, error) ||
        !get_components(destination, destination_root, destination_parts, error))
        return failure(error);

    auto source_parent = open_parent(source_parts, source_root, false, error);
    if (source_parent.descriptor < 0) return failure(error);
    auto destination_parent = open_parent(destination_parts, destination_root, false, error);
    if (destination_parent.descriptor < 0) return failure(error);

#ifdef SYS_renameat2
    const long status = ::syscall(
        SYS_renameat2, source_parent.descriptor, source_parent.leaf.c_str(),
        destination_parent.descriptor, destination_parent.leaf.c_str(),
        RENAME_NOREPLACE);
    if (status == 0) return {true, {}};
    return failure(system_error("Unable to rename without replacing an existing target"));
#else
    return failure("This system does not expose a race-safe no-replace rename operation.");
#endif
}

PlatformResult LinuxFilesystem::copy_authorized(
    const std::string& source, const std::string& destination,
    const permissions::AuthorizedRoot& source_root,
    const permissions::AuthorizedRoot& destination_root) {
    std::vector<std::string> source_parts, destination_parts;
    std::string error;
    if (!get_components(source, source_root, source_parts, error) ||
        !get_components(destination, destination_root, destination_parts, error))
        return failure(error);

    auto source_parent = open_parent(source_parts, source_root, false, error);
    if (source_parent.descriptor < 0) return failure(error);
    auto destination_parent = open_parent(destination_parts, destination_root, false, error);
    if (destination_parent.descriptor < 0) return failure(error);

    const int input = ::openat(source_parent.descriptor, source_parent.leaf.c_str(),
                               O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (input < 0) return failure(system_error("Unable to safely open the source file"));
    struct stat source_info {};
    if (::fstat(input, &source_info) < 0 || !S_ISREG(source_info.st_mode)) {
        ::close(input);
        return failure("The copy source must be a regular file.");
    }

    const int output = ::openat(destination_parent.descriptor,
                                destination_parent.leaf.c_str(),
                                O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                                0644);
    if (output < 0) {
        ::close(input);
        return failure(system_error("Unable to create the destination file safely"));
    }

    char buffer[16384];
    bool success = true;
    while (true) {
        const auto bytes = ::read(input, buffer, sizeof(buffer));
        if (bytes == 0) break;
        if (bytes < 0) {
            if (errno == EINTR) continue;
            error = system_error("Unable to read the source file");
            success = false;
            break;
        }
        ssize_t written = 0;
        while (written < bytes) {
            const auto count = ::write(output, buffer + written,
                                       static_cast<std::size_t>(bytes - written));
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) {
                error = system_error("Unable to write the destination file");
                success = false;
                break;
            }
            written += count;
        }
        if (!success) break;
    }
    if (::close(input) < 0 && success) {
        error = system_error("Unable to close the source file");
        success = false;
    }
    if (::close(output) < 0 && success) {
        error = system_error("Unable to close the destination file");
        success = false;
    }
    if (!success) {
        ::unlinkat(destination_parent.descriptor, destination_parent.leaf.c_str(), 0);
        return failure(error);
    }
    return {true, {}};
}

PlatformResult LinuxFilesystem::remove_authorized(
    const std::string& path, const permissions::AuthorizedRoot& root) {
    std::vector<std::string> components;
    std::string error;
    if (!get_components(path, root, components, error)) return failure(error);
    auto parent = open_parent(components, root, false, error);
    if (parent.descriptor < 0) return failure(error);

    struct stat info {};
    if (::fstatat(parent.descriptor, parent.leaf.c_str(), &info,
                  AT_SYMLINK_NOFOLLOW) < 0) {
        return failure(system_error("Unable to inspect the deletion target safely"));
    }
    const int flags = S_ISDIR(info.st_mode) ? AT_REMOVEDIR : 0;
    if (::unlinkat(parent.descriptor, parent.leaf.c_str(), flags) < 0) {
        return failure(system_error("Unable to remove the requested target"));
    }
    return {true, {}};
}

std::optional<std::string> LinuxFilesystem::read_text_authorized(
    const std::string& path, const permissions::AuthorizedRoot& root,
    std::string& error) const {
    constexpr std::size_t max_bytes = 4 * 1024 * 1024;
    std::vector<std::string> components;
    if (!get_components(path, root, components, error)) return std::nullopt;
    auto parent = open_parent(components, root, false, error);
    if (parent.descriptor < 0) return std::nullopt;

    const int descriptor = ::openat(parent.descriptor, parent.leaf.c_str(),
                                    O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0) {
        error = system_error("Unable to safely open the text file");
        return std::nullopt;
    }
    struct stat info {};
    if (::fstat(descriptor, &info) < 0 || !S_ISREG(info.st_mode)) {
        ::close(descriptor);
        error = "The target is not a regular file.";
        return std::nullopt;
    }
    if (info.st_size < 0 || static_cast<std::uintmax_t>(info.st_size) > max_bytes) {
        ::close(descriptor);
        error = "The file exceeds the 4 MiB text-read limit.";
        return std::nullopt;
    }

    std::string content;
    content.reserve(static_cast<std::size_t>(info.st_size));
    char buffer[16384];
    while (true) {
        const auto bytes = ::read(descriptor, buffer, sizeof(buffer));
        if (bytes == 0) break;
        if (bytes < 0) {
            if (errno == EINTR) continue;
            error = system_error("Unable to read the text file");
            ::close(descriptor);
            return std::nullopt;
        }
        if (content.size() + static_cast<std::size_t>(bytes) > max_bytes) {
            error = "The file exceeds the 4 MiB text-read limit.";
            ::close(descriptor);
            return std::nullopt;
        }
        content.append(buffer, static_cast<std::size_t>(bytes));
    }
    ::close(descriptor);
    return content;
}

} // namespace seventh::platform::linux_backend
