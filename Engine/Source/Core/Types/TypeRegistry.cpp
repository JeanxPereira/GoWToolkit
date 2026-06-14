#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Services/Logger.h>
#include <cstring>

namespace Onyx::Types {

TypeRegistry& TypeRegistry::Get() {
    static TypeRegistry instance;
    return instance;
}

void TypeRegistry::RegisterByMagic(GameVersion ver, std::unique_ptr<ITypeHandler> handler) {
    if (!handler) return;

    uint32_t magic = handler->GetMagic();
    TypeId id = handler->GetId();

    ITypeHandler* raw = handler.get();
    m_owned.push_back(std::move(handler));
    m_allHandlers.push_back(raw);

    uint64_t key = MakeKey(ver, magic);
    if (m_magicMap.count(key)) {
        LOG_WARN("[TypeRegistry] Overriding magic handler 0x%08X for version %d", magic, (int)ver);
    }
    m_magicMap[key] = raw;

    // Also register in the id map (first registration wins)
    uint32_t idKey = id.value;
    if (!m_idMap.count(idKey)) {
        m_idMap[idKey] = raw;
    }

    LOG_INFO("[TypeRegistry] Registered magic handler: %s (0x%08X) for version %d",
             raw->GetName(), magic, (int)ver);
}

void TypeRegistry::RegisterByTag(GameVersion ver, uint16_t tagNum, std::unique_ptr<ITypeHandler> handler) {
    if (!handler) return;

    TypeId id = handler->GetId();

    ITypeHandler* raw = handler.get();
    m_owned.push_back(std::move(handler));
    m_allHandlers.push_back(raw);

    uint64_t key = MakeKey(ver, tagNum);
    if (m_tagMap.count(key)) {
        LOG_WARN("[TypeRegistry] Overriding tag handler %d for version %d", tagNum, (int)ver);
    }
    m_tagMap[key] = raw;

    // Also register in the id map
    uint32_t idKey = id.value;
    if (!m_idMap.count(idKey)) {
        m_idMap[idKey] = raw;
    }

    LOG_INFO("[TypeRegistry] Registered tag handler: %s (tag=%d) for version %d",
             raw->GetName(), tagNum, (int)ver);
}

void TypeRegistry::RegisterByTypeId(std::unique_ptr<ITypeHandler> handler) {
    if (!handler) return;

    ITypeHandler* raw = handler.get();
    m_owned.push_back(std::move(handler));
    m_allHandlers.push_back(raw);

    uint32_t idKey = raw->GetId().value;
    m_idMap[idKey] = raw;  // Always overwrites â€” one canonical handler per TypeId

    LOG_INFO("[TypeRegistry] Registered file-level handler: %s (TypeId=%d)",
             raw->GetName(), idKey);
}

ITypeHandler* TypeRegistry::ResolveByTag(GameVersion ver, uint16_t tagNum,
                                          const uint8_t* payload, size_t payloadSize) const {
    // 1. Check tag-based map first (structural tags)
    uint64_t tagKey = MakeKey(ver, tagNum);
    auto tagIt = m_tagMap.find(tagKey);
    if (tagIt != m_tagMap.end()) {
        return tagIt->second;
    }

    // 2. If tag is SERVER_INSTANCE, dispatch by magic from payload
    if (tagNum == TAG_SERVER_INSTANCE && payload && payloadSize >= 4) {
        uint32_t magic = 0;
        std::memcpy(&magic, payload, 4);

        uint64_t magicKey = MakeKey(ver, magic);
        LOG_DEBUG("[TypeRegistry] ResolveByTag TAG_SERVER_INSTANCE -> magic=0x%08X key=0x%llX", magic, magicKey);

        auto magicIt = m_magicMap.find(magicKey);
        if (magicIt != m_magicMap.end()) {
            return magicIt->second;
        } else {
            LOG_DEBUG("[TypeRegistry] ResolveByTag FAILED to find handler for magic=0x%08X", magic);
        }
    }

    return nullptr;
}

ITypeHandler* TypeRegistry::Resolve(TypeId id) const {
    auto it = m_idMap.find(id.value);
    return (it != m_idMap.end()) ? it->second : nullptr;
}

} // namespace Onyx::Types
