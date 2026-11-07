#pragma once

#include "seventh/core/subsystem.hpp"
#include "seventh/engines/grammar/grammar_engine.hpp"

namespace seventh::grammar {

class GrammarSubsystem final : public Subsystem {
public:
    [[nodiscard]] std::string_view id() const noexcept override;
    [[nodiscard]] bool can_handle(const SubsystemRequest& request) const override;
    [[nodiscard]] SubsystemResult handle(const SubsystemRequest& request) override;

private:
    GrammarEngine engine_;
};

} // namespace seventh::grammar
