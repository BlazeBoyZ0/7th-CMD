#include "seventh/engines/grammar/grammar_parser.hpp"
#include "seventh/engines/grammar/entity_extractor.hpp"
#include <algorithm>
#include <cctype>
#include <set>
namespace seventh::grammar {
namespace {
std::string lower(std::string s) { for(char& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; }
std::size_t find_any(const std::vector<std::string>& t,const std::vector<std::string>& markers,std::size_t from) {
    for(std::size_t i=from;i<t.size();++i) for(const auto& m:markers) if(lower(t[i])==m)return i;
    return t.size();
}
std::string join(const std::vector<std::string>& t,std::size_t a,std::size_t b) {
    std::string s; for(std::size_t i=a;i<b&&i<t.size();++i){if(!s.empty())s+=' ';s+=t[i];} return s;
}
std::size_t skip_articles(const std::vector<std::string>& t,std::size_t i) {
    while(i<t.size()&&(lower(t[i])=="a"||lower(t[i])=="an"||lower(t[i])=="the"||lower(t[i])=="new"))++i;
    return i;
}
void put(ParseMatch& m,const std::string& key,const std::vector<std::string>& t,std::size_t a,std::size_t b) {
    a=std::min(a,t.size()); b=std::min(b,t.size());
    while(a<b&&(lower(t[a])=="a"||lower(t[a])=="an"||lower(t[a])=="the"))++a;
    if(a<b)m.slots[key]={join(t,a,b),true,a,b};
}
const IntentPattern* pattern_for(const GrammarConfig& c,const std::string& op,const std::string& entity) {
    for(const auto& p:c.patterns) if(p.operation==op&&(p.entity_type.empty()||p.entity_type==entity))return &p;
    return nullptr;
}
}
ParseMatch GrammarParser::parse(const std::vector<std::string>& tokens) const { return parse(tokens,GrammarConfig::defaults()); }
ParseMatch GrammarParser::parse(const std::vector<std::string>& input,const GrammarConfig& config) const {
    ParseMatch m; m.tokens=input;
    if(input.empty()){m.error="The instruction is empty.";return m;}
    for(auto& t:m.tokens)t=config.canonical_term(t);
    m.operation=config.canonical_term(m.tokens.front());
    if(!config.actions.contains(m.operation)){m.error="I do not recognize that command yet.";return m;}
    m.tokens.front()=m.operation; m.recognized=true;
    const auto entity=EntityExtractor{}.extract(m.tokens,config,1);
    m.entity_type=entity.found?entity.type:(m.operation=="open"?"application":"item");
    const auto* p=pattern_for(config,m.operation,m.entity_type);
    if(p)m.pattern_id=p->id;
    const std::size_t after_entity=entity.found?entity.token_index+1:1;
    const std::vector<std::string> locations=p?p->location_markers:std::vector<std::string>{"on","in","inside","within"};
    if(m.operation=="create") {
        const auto loc=find_any(m.tokens,locations,after_entity);
        const auto named=find_any(m.tokens,p?p->target_markers:std::vector<std::string>{"named"},after_entity);
        if(named<m.tokens.size()&&(loc==m.tokens.size()||named<loc)) {
            put(m,"name",m.tokens,named+1,loc);
            if(loc<m.tokens.size())put(m,"destination",m.tokens,loc+1,m.tokens.size());
        } else if(named<m.tokens.size()&&loc<m.tokens.size()) {
            put(m,"destination",m.tokens,loc+1,named);
            put(m,"name",m.tokens,named+1,m.tokens.size());
        } else {
            put(m,"name",m.tokens,skip_articles(m.tokens,after_entity),loc);
            if(loc<m.tokens.size())put(m,"destination",m.tokens,loc+1,m.tokens.size());
        }
        if(!m.slots.contains("name"))m.missing_slots.push_back("name");
    } else if(m.operation=="rename"||m.operation=="move"||m.operation=="copy") {
        const auto sep=find_any(m.tokens,p?p->destination_markers:std::vector<std::string>{"to"},after_entity);
        put(m,"source",m.tokens,skip_articles(m.tokens,after_entity),sep);
        if(sep<m.tokens.size())put(m,"destination",m.tokens,sep+1,m.tokens.size());
        if(!m.slots.contains("source"))m.missing_slots.push_back("source");
        if(!m.slots.contains("destination"))m.missing_slots.push_back("destination");
    } else if(m.operation=="delete"||m.operation=="open") {
        put(m,"target",m.tokens,skip_articles(m.tokens,after_entity),m.tokens.size());
        if(!m.slots.contains("target"))m.missing_slots.push_back("target");
    } else if(m.operation=="search") {
        const auto qmark=find_any(m.tokens,p?p->query_markers:std::vector<std::string>{"named","for"},after_entity);
        const auto qstart=qmark<m.tokens.size()?qmark+1:skip_articles(m.tokens,after_entity);
        const auto loc=find_any(m.tokens,locations,qstart);
        put(m,"query",m.tokens,qstart,loc);
        if(loc<m.tokens.size())put(m,"destination",m.tokens,loc+1,m.tokens.size());
        if(!m.slots.contains("query"))m.missing_slots.push_back("query");
    } else if(m.operation=="count") {
        static const std::set<std::string> metrics{"word","words","line","lines","character","characters","byte","bytes"};
        std::size_t metric=m.tokens.size();
        for(std::size_t i=1;i<m.tokens.size()&&i<8;++i)if(metrics.contains(lower(m.tokens[i]))){metric=i;break;}
        if(metric<m.tokens.size()) {
            std::string v=lower(m.tokens[metric]); if(v=="word")v="words";if(v=="line")v="lines";if(v=="character")v="characters";if(v=="byte")v="bytes";
            m.slots["metric"]={v,true,metric,metric+1};
            const auto file=find_any(m.tokens,{"in","of","from"},metric+1);
            put(m,"file",m.tokens,file<m.tokens.size()?file+1:metric+1,m.tokens.size());
        }
        if(!m.slots.contains("metric"))m.missing_slots.push_back("metric");
        if(!m.slots.contains("file"))m.missing_slots.push_back("file");
    }
    m.complete=m.missing_slots.empty();
    return m;
}
} // namespace seventh::grammar
