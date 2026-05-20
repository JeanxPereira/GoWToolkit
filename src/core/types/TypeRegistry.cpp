#include "TypeRegistry.h"
#include "core/Logger.h"
#include <cstring>

namespace GOW {

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
    uint32_t idKey = static_cast<uint32_t>(id);
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
    uint32_t idKey = static_cast<uint32_t>(id);
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

    uint32_t idKey = static_cast<uint32_t>(raw->GetId());
    m_idMap[idKey] = raw;  // Always overwrites — one canonical handler per TypeId

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
    auto it = m_idMap.find(static_cast<uint32_t>(id));
    return (it != m_idMap.end()) ? it->second : nullptr;
}

// ── TypeIdName ──────────────────────────────────────────────────────────────

const char* TypeIdName(TypeId id) {
    switch (id) {
        case TypeId::Unknown:        return "Unknown";
        case TypeId::EntityCount:    return "Entity Count";
        case TypeId::GroupStart:     return "Group Start";
        case TypeId::GroupEnd:       return "Group End";
        case TypeId::HeaderStart:    return "Header Start";
        case TypeId::HeaderPop:      return "Header Pop";
        case TypeId::Instance:       return "Instance";
        case TypeId::Object:         return "Object";
        case TypeId::Model:          return "Model";
        case TypeId::Mesh:           return "Mesh";
        case TypeId::Material:       return "Material";
        case TypeId::Texture:        return "Texture";
        case TypeId::GfxData:        return "GFX Data";
        case TypeId::PalData:        return "Palette";
        case TypeId::Animation:      return "Animation";
        case TypeId::Script:         return "Script";
        case TypeId::Light:          return "Light";
        case TypeId::Sound:          return "Sound";
        case TypeId::Collision:      return "Collision";
        case TypeId::Flipbook:       return "Flipbook";
        case TypeId::Chunk:          return "Chunk";
        case TypeId::WadFile:        return "WAD File";
        case TypeId::VagAudio:       return "VAG Audio";
        case TypeId::VpkVideo:       return "VPK Video";
        case TypeId::PssVideo:       return "PSS Video";
        case TypeId::PswVideo:       return "PSW Video";
        case TypeId::TextPlain:      return "Text";
        case TypeId::ShaderContainer: return "Shader";
        case TypeId::ShaderVertex:   return "Vertex Shader";
        case TypeId::ShaderPixel:    return "Pixel Shader";
        case TypeId::ShaderHull:     return "Hull Shader";
        case TypeId::ShaderDomain:   return "Domain Shader";
        case TypeId::ShaderCompute:  return "Compute Shader";
        case TypeId::ShaderLibrary:  return "Library Shader";
        case TypeId::MeshGpu:        return "Mesh GPU";
        case TypeId::MeshDefn:       return "Mesh Definition";
        case TypeId::GameObjectProto: return "GO Proto";
        case TypeId::GameObjectInst: return "GO Instance";
        case TypeId::GameObjectOverride: return "GO Override";
        case TypeId::TexturePair:    return "Texture Pair";
        case TypeId::MaterialRef:    return "Material Ref";
        case TypeId::LodBinding:     return "LOD Binding";
        case TypeId::AnimClip:       return "Anim Clip";
        case TypeId::SoundEmitter:   return "Sound Emitter";
        case TypeId::ParticleEmitter: return "Particle Emitter";
        case TypeId::ParticleSystem: return "Particle System";
        case TypeId::ClientGuid:     return "Client GUID";
        case TypeId::WadIdentity:    return "WAD Identity";
        case TypeId::SharedWadRef:   return "Shared WAD Ref";
        case TypeId::Sentinel:       return "Sentinel";
        default:                     return "Unknown";
    }
}

} // namespace GOW
