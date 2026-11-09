#include "seventh/systems/information/provider_registry.hpp"

#include <utility>

namespace seventh::information {

void ProviderRegistry::register_provider(std::shared_ptr<DataProvider> provider) {
    if (!provider || provider->id().empty()) return;
    for (auto& registered : providers_) {
        if (registered->id() == provider->id()) {
            registered = std::move(provider);
            return;
        }
    }
    providers_.push_back(std::move(provider));
}

std::shared_ptr<DataProvider> ProviderRegistry::find(
    const DataRequest& request) const {
    for (const auto& provider : providers_) {
        if ((request.provider_id.empty() || provider->id() == request.provider_id) &&
            provider->supports(request)) {
            return provider;
        }
    }
    return {};
}

} // namespace seventh::information
