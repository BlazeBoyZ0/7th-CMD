#include "seventh/systems/information/data_acquisition.hpp"

namespace seventh::information {

DataAcquisition::DataAcquisition(const ProviderRegistry& registry)
    : registry_(registry) {}

bool DataAcquisition::acquire(const DataRequest& request,
                              std::vector<DataRecord>& records,
                              std::string& error) const {
    const auto provider = registry_.find(request);
    if (!provider) {
        error = "No registered data provider supports this request.";
        return false;
    }
    records = provider->acquire(request);
    return true;
}

} // namespace seventh::information
