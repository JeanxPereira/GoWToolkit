// Structural WAD tag handlers â€” identified by tag number, not magic.
// These have no payload to parse, they just control the WAD tree structure.

#include "Core/Types/TypeRegistry.h"
#include "Core/Types/ITypeHandler.h"
#include "core/types/GameTypes.h"
#include "Fonts/SFSymbols.h"

namespace {

class EntityCountHandler : public Onyx::ITypeHandler {
public:
    Onyx::TypeId  GetId()    const override { return Onyx::GameTypes::EntityCount; }
    const char*  GetName()  const override { return "Entity Count"; }
    uint32_t     GetMagic() const override { return 0; }
    const char*  GetIcon()  const override { return ICON_SF_MINUS; }  // dash
    Color4f      GetColor() const override { return {0.4f, 0.4f, 0.4f, 1.0f}; }
};

class GroupStartHandler : public Onyx::ITypeHandler {
public:
    Onyx::TypeId  GetId()    const override { return Onyx::GameTypes::GroupStart; }
    const char*  GetName()  const override { return "Group"; }
    uint32_t     GetMagic() const override { return 0; }
    const char*  GetIcon()  const override { return ICON_SF_FOLDER_FILL; }  // folder
    Color4f      GetColor() const override { return {0.9f, 0.9f, 0.9f, 1.0f}; }
};

class GroupEndHandler : public Onyx::ITypeHandler {
public:
    Onyx::TypeId  GetId()    const override { return Onyx::GameTypes::GroupEnd; }
    const char*  GetName()  const override { return "Group End"; }
    uint32_t     GetMagic() const override { return 0; }
    const char*  GetIcon()  const override { return ICON_SF_MINUS; }
    Color4f      GetColor() const override { return {0.4f, 0.4f, 0.4f, 1.0f}; }
};

class HeaderStartHandler : public Onyx::ITypeHandler {
public:
    Onyx::TypeId  GetId()    const override { return Onyx::GameTypes::HeaderStart; }
    const char*  GetName()  const override { return "Header Start"; }
    uint32_t     GetMagic() const override { return 0; }
};

class HeaderPopHandler : public Onyx::ITypeHandler {
public:
    Onyx::TypeId  GetId()    const override { return Onyx::GameTypes::HeaderPop; }
    const char*  GetName()  const override { return "Header Pop"; }
    uint32_t     GetMagic() const override { return 0; }
};

} // anonymous namespace

// â”€â”€ Self-registration for GOW2 â”€â”€
REGISTER_TAG(GOW2, 0,  EntityCountHandler);
REGISTER_TAG(GOW2, 2,  GroupStartHandler);
REGISTER_TAG(GOW2, 3,  GroupEndHandler);
REGISTER_TAG(GOW2, 21, HeaderStartHandler);
REGISTER_TAG(GOW2, 19, HeaderPopHandler);

