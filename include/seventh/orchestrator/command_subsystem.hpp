#pragma once

#include "seventh/core/subsystem.hpp"
#include "seventh/engines/execution/execution_engine.hpp"
#include "seventh/engines/grammar/grammar_config.hpp"
#include "seventh/engines/grammar/grammar_engine.hpp"
#include "seventh/engines/planning/planning_engine.hpp"
#include "seventh/engines/planning/plan_state_manager.hpp"
#include "seventh/engines/reasoning/reasoning_engine.hpp"
#include "seventh/platform/platform_interface.hpp"
#include "seventh/systems/capabilities/capability_registry.hpp"
#include "seventh/systems/entity_resolution/entity_resolver.hpp"
#include "seventh/systems/knowledge/context_manager.hpp"
#include "seventh/systems/knowledge/knowledge_system.hpp"
#include "seventh/systems/permissions/permission_manager.hpp"
#include "seventh/systems/information/provider_registry.hpp"
#include "seventh/systems/verification/verification_system.hpp"

#include <vector>

namespace seventh {

class CommandSubsystem final : public Subsystem {
public:
    explicit CommandSubsystem(
        platform::PlatformInterface& platform,
        grammar::GrammarConfig grammar_config = grammar::GrammarConfig::defaults(),
        knowledge::KnowledgeSystem* knowledge = nullptr);

    [[nodiscard]] std::string_view id() const noexcept override;
    [[nodiscard]] bool can_handle(const SubsystemRequest& request) const override;
    [[nodiscard]] SubsystemResult handle(const SubsystemRequest& request) override;
    bool load_capabilities(const std::string& path, std::string& error);
    bool load_reasoning_rules(const std::string& path, std::string& error,
                              bool merge = false);

private:
    [[nodiscard]] std::string resolve_destination(const std::string& destination) const;
    void remember_folder(const std::string& name, const std::string& path);
    [[nodiscard]] SubsystemResult create_folder(
        const SubsystemRequest& request, const grammar::CanonicalIntent& intent);
    [[nodiscard]] SubsystemResult open_target(
        const SubsystemRequest& request, const grammar::CanonicalIntent& intent);
    [[nodiscard]] SubsystemResult search_items(
        const SubsystemRequest& request, const grammar::CanonicalIntent& intent);
    [[nodiscard]] SubsystemResult modify_path(
        const SubsystemRequest& request, const grammar::CanonicalIntent& intent);
    [[nodiscard]] SubsystemResult delete_target(
        const SubsystemRequest& request, const grammar::CanonicalIntent& intent);
    [[nodiscard]] SubsystemResult count_file(
        const SubsystemRequest& request, const grammar::CanonicalIntent& intent);
    [[nodiscard]] SubsystemResult create_folder_chain(
        const SubsystemRequest& request, const std::vector<std::string>& clauses);
    [[nodiscard]] SubsystemResult handle_single(
        const SubsystemRequest& request, const std::string& raw_input);

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
    information::ProviderRegistry information_registry_;
    knowledge::ContextManager context_manager_;
    knowledge::KnowledgeSystem* knowledge_{nullptr};
};

} // namespace seventh
