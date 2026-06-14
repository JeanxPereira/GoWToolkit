#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Fonts/SFSymbols.h>

namespace Onyx::Types {

TypeCatalog& TypeCatalog::Get() { static TypeCatalog inst; return inst; }

TypeCatalog::TypeCatalog() {
    m_unknown.key = "UNKNOWN";
    m_unknown.label = "Unknown";
    m_infos.push_back(m_unknown);            // index 0 == Unknown
    m_byKey[m_unknown.key] = TypeId{0};
}

TypeId TypeCatalog::Register(const TypeInfo& info, uint32_t forcedValue) {
    if (auto it = m_byKey.find(info.key); it != m_byKey.end()) return it->second;
    uint32_t v = forcedValue ? forcedValue : (uint32_t)m_infos.size();
    if (v >= m_infos.size()) m_infos.resize(v + 1);
    m_infos[v] = info;
    TypeId id{v};
    m_byKey[info.key] = id;
    return id;
}

TypeId TypeCatalog::Find(std::string_view key) const {
    auto it = m_byKey.find(std::string(key));
    return it == m_byKey.end() ? TypeId{} : it->second;
}

const TypeInfo& TypeCatalog::Info(TypeId id) const {
    return (id.value < m_infos.size()) ? m_infos[id.value] : m_unknown;
}

const char* TypeCatalog::Icon(TypeId id) const {
    const char* ic = Info(id).icon;
    return ic ? ic : ICON_SF_DOCUMENT;
}

void TypeCatalog::Color(TypeId id, float out[4]) const {
    const auto& c = Info(id).color;
    out[0]=c[0]; out[1]=c[1]; out[2]=c[2]; out[3]=c[3];
}

} // namespace Onyx::Types
