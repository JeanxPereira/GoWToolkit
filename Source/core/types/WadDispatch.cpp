#include "core/types/WadDispatch.h"
#include <cstring>

namespace Onyx::Gow {

WadTypeRegistry& WadTypeRegistry::Get() {
    static WadTypeRegistry instance;
    return instance;
}

void WadTypeRegistry::RegisterByMagic(GameVersion ver, uint32_t magic, IWadTypeHandler* handler) {
    if (!handler) return;
    m_magicMap[MakeKey(ver, magic)] = handler;
}

void WadTypeRegistry::RegisterByTag(GameVersion ver, uint16_t tagNum, IWadTypeHandler* handler) {
    if (!handler) return;
    m_tagMap[MakeKey(ver, tagNum)] = handler;
}

IWadTypeHandler* WadTypeRegistry::ResolveByTag(GameVersion ver, uint16_t tagNum,
                                               const uint8_t* payload, size_t payloadSize) const {
    auto tagIt = m_tagMap.find(MakeKey(ver, tagNum));
    if (tagIt != m_tagMap.end()) return tagIt->second;

    if (tagNum == TAG_SERVER_INSTANCE && payload && payloadSize >= 4) {
        uint32_t magic = 0;
        std::memcpy(&magic, payload, 4);
        auto magicIt = m_magicMap.find(MakeKey(ver, magic));
        if (magicIt != m_magicMap.end()) return magicIt->second;
    }
    return nullptr;
}

} // namespace Onyx::Gow
