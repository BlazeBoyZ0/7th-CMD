#pragma once

#include "seventh/systems/information/provider_registry.hpp"

namespace seventh::information {

class DataAcquisition {
public:
    explicit DataAcquisition(const ProviderRegistry& registry);
    [[nodiscard]] bool acquire(const DataRequest& request,
                               std::vector<DataRecord>& records,
                               std::string& error) const;

private:
    const ProviderRegistry& registry_;
};

} // namespace seventh::information
