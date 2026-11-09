#pragma once

#include "seventh/systems/information/data_acquisition.hpp"
#include "seventh/systems/information/data_normalizer.hpp"
#include "seventh/systems/information/operation_engine.hpp"
#include "seventh/systems/information/result_builder.hpp"

namespace seventh::information {

class InformationProcessor {
public:
    explicit InformationProcessor(const ProviderRegistry& registry);
    [[nodiscard]] DataResult process(const DataRequest& request) const;

private:
    DataAcquisition acquisition_;
    DataNormalizer normalizer_;
    OperationEngine operations_;
    ResultBuilder result_builder_;
};

} // namespace seventh::information
