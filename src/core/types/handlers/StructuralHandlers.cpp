// Structural WAD tag handlers — identified by tag number, not magic.
// These have no payload to parse, they just control the WAD tree structure.

#include "core/types/TypeRegistry.h"
#include "core/types/ITypeHandler.h"
#include "fonts/SFSymbols.h"

namespace {

class EntityCountHandler : public GOW::ITypeHandler {
public:
    GOW::TypeId  GetId()    const override { return GOW::TypeId::EntityCount; }
    const char*  GetName()  const override { return "Entity Count"; }
    uint32_t     GetMagic() const override { return 0; }
    const char*  GetIcon()  const override { return ICON_SF_MINUS; }  // dash
    Color4f      GetColor() const override { return {0.4f, 0.4f, 0.4f, 1.0f}; }
};

class GroupStartHandler : public GOW::ITypeHandler {
public:
    GOW::TypeId  GetId()    const override { return GOW::TypeId::GroupStart; }
    const char*  GetName()  const override { return "Group"; }
    uint32_t     GetMagic() const override { return 0; }
    const char*  GetIcon()  const override { return ICON_SF_FOLDER_FILL; }  // folder
    Color4f      GetColor() const override { return {0.9f, 0.9f, 0.9f, 1.0f}; }
};

class GroupEndHandler : public GOW::ITypeHandler {
public:
    GOW::TypeId  GetId()    const override { return GOW::TypeId::GroupEnd; }
    const char*  GetName()  const override { return "Group End"; }
    uint32_t     GetMagic() const override { return 0; }
    const char*  GetIcon()  const override { return ICON_SF_MINUS; }
    Color4f      GetColor() const override { return {0.4f, 0.4f, 0.4f, 1.0f}; }
};

class HeaderStartHandler : public GOW::ITypeHandler {
public:
    GOW::TypeId  GetId()    const override { return GOW::TypeId::HeaderStart; }
    const char*  GetName()  const override { return "Header Start"; }
    uint32_t     GetMagic() const override { return 0; }
};

class HeaderPopHandler : public GOW::ITypeHandler {
public:
    GOW::TypeId  GetId()    const override { return GOW::TypeId::HeaderPop; }
    const char*  GetName()  const override { return "Header Pop"; }
    uint32_t     GetMagic() const override { return 0; }
};

} // anonymous namespace

// ── Self-registration for GOW2 ──
REGISTER_TAG(GOW2, 0,  EntityCountHandler);
REGISTER_TAG(GOW2, 2,  GroupStartHandler);
REGISTER_TAG(GOW2, 3,  GroupEndHandler);
REGISTER_TAG(GOW2, 21, HeaderStartHandler);
REGISTER_TAG(GOW2, 19, HeaderPopHandler);

// ── Same structural tags for GOW1 ──
// Use separate static bools to avoid name collisions
static bool _reg_GOW1_EntityCount = [] {
    GOW::TypeRegistry::Get().RegisterByTag(GOW::GameVersion::GOW1, 0,
        std::make_unique<EntityCountHandler>());
    return true;
}();
static bool _reg_GOW1_GroupStart = [] {
    GOW::TypeRegistry::Get().RegisterByTag(GOW::GameVersion::GOW1, 2,
        std::make_unique<GroupStartHandler>());
    return true;
}();
static bool _reg_GOW1_GroupEnd = [] {
    GOW::TypeRegistry::Get().RegisterByTag(GOW::GameVersion::GOW1, 3,
        std::make_unique<GroupEndHandler>());
    return true;
}();
static bool _reg_GOW1_HeaderStart = [] {
    GOW::TypeRegistry::Get().RegisterByTag(GOW::GameVersion::GOW1, 21,
        std::make_unique<HeaderStartHandler>());
    return true;
}();
static bool _reg_GOW1_HeaderPop = [] {
    GOW::TypeRegistry::Get().RegisterByTag(GOW::GameVersion::GOW1, 19,
        std::make_unique<HeaderPopHandler>());
    return true;
}();
