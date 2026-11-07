#pragma once

#include "seventh/systems/information/information_types.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace seventh::information {

class DataProvider {
public:
    virtual ~DataProvider() = default;
    [[nodiscard]] virtual std::string_view id() const noexcept = 0;
    [[nodiscard]] virtual bool supports(const DataRequest& request) const = 0;
    [[nodiscard]] virtual bool acquire(
        const DataRequest& request, std::vector<DataRecord>& records,
        std::string& error) = 0;
};

class ProviderRegistry {
public:
    void register_provider(std::shared_ptr<DataProvider> provider);
    [[nodiscard]] std::shared_ptr<DataProvider> find(
        const DataRequest& request) const;

private:
    std::vector<std::shared_ptr<DataProvider>> providers_;
};

} // namespace seventh::information
