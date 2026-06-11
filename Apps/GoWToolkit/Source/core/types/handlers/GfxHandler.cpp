// GFX handler â€” raw GS pixel data
// Magic: 0x0C

#include "Core/Types/ITypeHandler.h"
#include "Core/Types/TypeRegistry.h"
#include "core/types/GameTypes.h"

#include "Fonts/SFSymbols.h"

namespace {

class GfxHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::GfxData; }
  const char *GetName() const override { return "GFX Data"; }
  uint32_t GetMagic() const override { return 0x0000000C; }
  const char *GetIcon() const override { return ICON_SF_PHOTO; } // file-media
  Color4f GetColor() const override { return {1.0f, 0.5f, 0.8f, 1.0f}; }
};

} // namespace

REGISTER_TYPE(GOW2, GfxHandler);
