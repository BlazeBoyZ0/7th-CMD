#pragma once

#include "seventh/platform/platform_interface.hpp"
#include "seventh/systems/information/provider_registry.hpp"

namespace seventh::information {

// Registers read-only providers backed by the active platform abstraction.
void register_platform_data_providers(
    ProviderRegistry& registry, platform::PlatformInterface& platform);

} // namespace seventh::information
