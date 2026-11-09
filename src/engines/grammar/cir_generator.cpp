#include "seventh/engines/grammar/cir_generator.hpp"
#include <atomic>
#include <cstdint>
namespace seventh::grammar {
CanonicalIntent CirGenerator::generate(const AstNode& node,const std::string& raw) const {
    CanonicalIntent out; out.operation=node.operation; out.entity_type=node.entity_type;
    out.raw_input=raw; out.pattern_id=node.pattern_id; out.slots=node.slots;
    out.missing_information=node.missing_slots;
    out.relationships=node.relationships;
    const auto value=[&](const char* k)->std::string{const auto i=node.slots.find(k);return i==node.slots.end()?std::string{}:i->second.value;};
    out.name=value("name"); if(out.name.empty())out.name=value("target"); if(out.name.empty())out.name=value("file");
    out.source=value("source"); out.destination=value("destination"); out.query=value("query");
    if(out.operation=="count")out.query=value("metric");
    out.valid=node.valid&&out.missing_information.empty();
    if(!out.valid)out.error=node.error.empty()?"The instruction is missing required information.":node.error;
    static std::atomic<std::uint64_t> next_id{1};
    out.id="cir-"+std::to_string(next_id.fetch_add(1,std::memory_order_relaxed));
    return out;
}
} // namespace seventh::grammar
