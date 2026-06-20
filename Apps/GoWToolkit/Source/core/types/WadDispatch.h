#pragma once
#include <Onyx/Types/ITypeHandler.h>
#include <Onyx/Types/TypeRegistry.h>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace Onyx::Gow {

// God of War WAD dispatch — game variant key for magic/tag handler lookup.
enum class GameVersion : uint8_t {
    GOW2,
    GOWR,
};

// Adds a binary "magic" (first 4 bytes of payload) on top of the generic
// Onyx handler. Tag-dispatched handlers inherit the default GetMagic()==0.
class IWadTypeHandler : public Onyx::Types::ITypeHandler {
public:
    virtual uint32_t GetMagic() const { return 0; }
};

// GoW-local dispatcher: (version, magic) and (version, tag) maps. Handlers are
// owned by Onyx::Types::TypeRegistry (registered there by TypeId); this registry
// holds non-owning pointers for parse-time lookup.
class WadTypeRegistry {
public:
    static WadTypeRegistry& Get();

    void RegisterByMagic(GameVersion ver, uint32_t magic, IWadTypeHandler* handler);
    void RegisterByTag(GameVersion ver, uint16_t tagNum, IWadTypeHandler* handler);

    // Tag map first; if tag == TAG_SERVER_INSTANCE, dispatch by 4-byte magic.
    IWadTypeHandler* ResolveByTag(GameVersion ver, uint16_t tagNum,
                                  const uint8_t* payload, size_t payloadSize) const;

private:
    WadTypeRegistry() = default;
    static uint64_t MakeKey(GameVersion ver, uint32_t value) {
        return (static_cast<uint64_t>(ver) << 32) | value;
    }
    static constexpr uint16_t TAG_SERVER_INSTANCE = 1;

    std::unordered_map<uint64_t, IWadTypeHandler*> m_magicMap;
    std::unordered_map<uint64_t, IWadTypeHandler*> m_tagMap;
};

} // namespace Onyx::Gow

// ── Self-registration: register into BOTH the Onyx TypeRegistry (by TypeId, so
//    the UI can resolve viewer/icon/color) and the GoW WadTypeRegistry. ────────
#define _GOW2_REG_CONCAT2(a, b) a##b
#define _GOW2_REG_CONCAT(a, b) _GOW2_REG_CONCAT2(a, b)

#define REGISTER_GOW_TYPE(version, HandlerClass)                                  \
  static bool _GOW2_REG_CONCAT(_gow_reg_##HandlerClass##_, __LINE__) = [] {       \
    auto h = std::make_unique<HandlerClass>();                                    \
    uint32_t _magic = h->GetMagic();                                              \
    ::Onyx::Gow::IWadTypeHandler* _raw = h.get();                                 \
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(std::move(h));            \
    ::Onyx::Gow::WadTypeRegistry::Get().RegisterByMagic(                          \
        ::Onyx::Gow::GameVersion::version, _magic, _raw);                         \
    return true;                                                                  \
  }()

#define REGISTER_GOW_TAG(version, tagNum, HandlerClass)                           \
  static bool _GOW2_REG_CONCAT(_gow_reg_tag_##HandlerClass##_, __LINE__) = [] {   \
    auto h = std::make_unique<HandlerClass>();                                    \
    ::Onyx::Gow::IWadTypeHandler* _raw = h.get();                                 \
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(std::move(h));            \
    ::Onyx::Gow::WadTypeRegistry::Get().RegisterByTag(                            \
        ::Onyx::Gow::GameVersion::version, tagNum, _raw);                         \
    return true;                                                                  \
  }()
