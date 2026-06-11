#pragma once
#include "../parsers/shared/SceneNode.h"
#include "../vfs/IFile.h"
#include "GameVersion.h"
#include "TypeId.h"
#include "Fonts/SFSymbols.h"
#include <cstddef>
#include <cstdint>
#include <memory>

// Forward declarations
namespace Onyx {
class AssetNode;
class IDocumentContent;
class SceneData;
} // namespace Onyx
struct AssetEntry;
struct AssetContainer;

namespace Onyx {

/// Abstract handler for a single asset type.
/// Each concrete handler is a self-contained unit that knows how to
/// identify, parse, and visualize one kind of asset.
class ITypeHandler {
public:
  virtual ~ITypeHandler() = default;

  /// Unique type identity (compile-time enum value).
  virtual TypeId GetId() const = 0;

  /// Human-readable name (e.g. "Model", "Material", "Instance").
  virtual const char *GetName() const = 0;

  /// Binary magic number from the first 4 bytes of the payload.
  /// Return 0 for structural tags that are dispatched by tag number instead.
  virtual uint32_t GetMagic() const = 0;

  /// Icon string for the tree view (UTF-8 codicon). Override to customize.
  virtual const char *GetIcon() const {
    return ICON_SF_DOCUMENT;
  } // default: file icon

  /// RGBA color for the tree view label. Override to customize.
  struct Color4f {
    float r, g, b, a;
  };
  virtual Color4f GetColor() const { return {0.6f, 0.6f, 0.6f, 1.0f}; }

  /// Parse raw payload bytes into a structured AssetNode tree for InfoTab
  /// display. Return nullptr if this type has no structured properties to show.
  virtual std::shared_ptr<AssetNode> Parse(std::shared_ptr<IFile> file) {
    (void)file;
    return nullptr;
  }

  /// Create a viewer (3D viewport, texture preview, audio player, etc.)
  /// for the given entry. Return nullptr if this type has no viewer.
  virtual std::shared_ptr<IDocumentContent>
  CreateViewer(const AssetEntry &entry, AssetContainer &wad) {
    (void)entry;
    (void)wad;
    return nullptr;
  }

  /// Extract scene data without generating a viewer (useful for composition
  /// layers like Chunks)
  virtual std::unique_ptr<Onyx::SceneData>
  BuildSceneData(const AssetEntry &entry, AssetContainer &wad) {
    (void)entry;
    (void)wad;
    return nullptr;
  }
};

} // namespace Onyx

// â”€â”€ Self-registration macro â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Usage (at the bottom of a handler .cpp file):
//   REGISTER_TYPE(GOW2, InstanceHandler);
//
// This creates a static bool whose initializer registers the handler
// at program startup, exactly like Go's init() functions.
//
// For tag-based handlers (structural tags with no magic), use:
//   REGISTER_TAG(GOW2, 0, EntityCountHandler);

#define _GOW_REG_CONCAT2(a, b) a##b
#define _GOW_REG_CONCAT(a, b) _GOW_REG_CONCAT2(a, b)

#define REGISTER_TYPE(version, HandlerClass)                                   \
  static bool _GOW_REG_CONCAT(_reg_##version##_##HandlerClass##_,              \
                              __LINE__) = [] {                                 \
    ::Onyx::TypeRegistry::Get().RegisterByMagic(                                \
        ::Onyx::GameVersion::version, std::make_unique<HandlerClass>());        \
    return true;                                                               \
  }()

#define REGISTER_TAG(version, tagNum, HandlerClass)                            \
  static bool _GOW_REG_CONCAT(_reg_tag_##version##_##HandlerClass##_,          \
                              __LINE__) = [] {                                 \
    ::Onyx::TypeRegistry::Get().RegisterByTag(                                  \
        ::Onyx::GameVersion::version, tagNum,                                   \
        std::make_unique<HandlerClass>());                                     \
    return true;                                                               \
  }()

// File-level handler registration (identified by extension, not magic).
// These handlers are registered by TypeId directly and never enter the magic
// map. Use for types like VAG, VPK, PSS, PSW that live as standalone files in
// PAK/TOC.
#define REGISTER_FILE_TYPE(HandlerClass)                                       \
  static bool _GOW_REG_CONCAT(_reg_ft_##HandlerClass##_, __LINE__) = [] {      \
    ::Onyx::TypeRegistry::Get().RegisterByTypeId(                               \
        std::make_unique<HandlerClass>());                                     \
    return true;                                                               \
  }()

// Forward-declare TypeRegistry so the macros compile
namespace Onyx {
class TypeRegistry;
}
