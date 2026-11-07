#pragma once

#include "seventh/platform/applications/application_interface.hpp"

namespace seventh::platform::linux_backend {

class LinuxApplications final : public ApplicationInterface {
public:
    [[nodiscard]] std::vector<ApplicationEntry> list(
        std::string& error) const override;
    [[nodiscard]] std::optional<ApplicationEntry> find(
        const std::string& name) const override;
    [[nodiscard]] std::optional<ApplicationEntry> find_default(
        const std::string& role) const override;
    [[nodiscard]] PlatformResult launch(
        const ApplicationEntry& application) override;
};

} // namespace seventh::platform::linux_backend
