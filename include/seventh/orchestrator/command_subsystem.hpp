#pragma once

#include "seventh/core/subsystem.hpp"
#include "seventh/engines/execution/execution_engine.hpp"
#include "seventh/engines/grammar/grammar_config.hpp"
#include "seventh/engines/planning/planning_engine.hpp"
#include "seventh/engines/planning/plan_state_manager.hpp"
#include "seventh/engines/reasoning/reasoning_engine.hpp"
#include "seventh/platform/platform_interface.hpp"
#include "seventh/systems/capabilities/capability_registry.hpp"
#include "seventh/systems/entity_resolution/entity_resolver.hpp"
#include "seventh/systems/permissions/permission_manager.hpp"
#include "seventh/systems/verification/verification_system.hpp"

namespace seventh {

class CommandSubsystem final : public Subsystem {
public:
    explicit CommandSubsystem(
        platform::PlatformInterface& platform,
        grammar::GrammarConfig grammar_config = grammar::GrammarConfig::defaults());

    [[nodiscard]] std::string_view id() const noexcept override;
    [[nodiscard]] bool can_handle(const SubsystemRequest& request) const override;
    [[nodiscard]] SubsystemResult handle(const SubsystemRequest& request) override;

private:
    [[nodiscard]] std::string resolve_destination(const std::string& destination) const;

    platform::PlatformInterface& platform_;
    grammar::GrammarEngine grammar_engine_;
    entity_resolution::EntityResolver entity_resolver_;
    reasoning::ReasoningEngine reasoning_engine_;
    planning::PlanningEngine planning_engine_;
    planning::PlanStateManager plan_state_manager_;
    capabilities::CapabilityRegistry capabilities_;
    permissions::PermissionManager permission_manager_;
    execution::ExecutionEngine execution_engine_;
    verification::VerificationSystem verification_system_;
};

} // namespace seventh
