// GFX handler — raw GS pixel data
// Magic: 0x0C

#include "core/types/ITypeHandler.h"
#include "core/types/TypeRegistry.h"

#include "fonts/SFSymbols.h"

namespace {

class GfxHandler : public GOW::ITypeHandler {
public:
  GOW::TypeId GetId() const override { return GOW::TypeId::GfxData; }
  const char *GetName() const override { return "GFX Data"; }
  uint32_t GetMagic() const override { return 0x0000000C; }
  const char *GetIcon() const override { return ICON_SF_PHOTO; } // file-media
  Color4f GetColor() const override { return {1.0f, 0.5f, 0.8f, 1.0f}; }
};

} // namespace

REGISTER_TYPE(GOW1, GfxHandler);
REGISTER_TYPE(GOW2, GfxHandler);
