#include "seventh/systems/information/data_acquisition.hpp"

#include <exception>

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

    try {
        records.clear();
        if (!provider->acquire(request, records, error)) {
            if (error.empty()) error = "The data provider failed without an explanation.";
            records.clear();
            return false;
        }
    } catch (const std::exception& exception) {
        records.clear();
        error = std::string("The data provider raised an exception: ") + exception.what();
        return false;
    } catch (...) {
        records.clear();
        error = "The data provider raised an unknown exception.";
        return false;
    }

    error.clear();
    return true;
}

} // namespace seventh::information
