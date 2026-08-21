#pragma once

// Shared GOW2 tree-walking and material staging.
//
// ObjectHandler and ModelHandler both build a Parsers::SceneData out of a GOW2
// Model node, and both carried byte-identical copies of ResolveRef,
// ResolvePayload, FindTexture and SelectMainLayer. The copies are gone; this
// is the one implementation.
//
// Two things changed under Onyx v1.1 and both live here:
//
//   1. Materials. The removed model was positional -- MaterialInfo::Layer[]
//      with "layer 0 is the one that draws", plus a nested
//      SceneData::textures shaped vector<vector<unique_ptr<TextureData>>>.
//      v1.1 is explicit: MaterialDesc::textures is a map<TextureRole, int>
//      indexing a FLAT SceneData::textures pool, and a role absent from the
//      map means "no map for that role" -- never index 0.
//
//   2. Type identity. Phase 2 Task 4 converged Gow2Module's tree production
//      onto its own TypeRegistrar-minted handles, which are catalog keys
//      prefixed by the module id ("gow2.texture"). The legacy
//      Onyx::GameTypes::Texture extern is a DIFFERENT id registered under the
//      bare key "texture", so `entry.typeId == GameTypes::Texture` silently
//      never matches a tree Gow2Module actually produced. Every type test in
//      this file goes through SceneTypes instead.

#include "core/parsers/gow2/MaterialParser.h"

#include <Onyx/Domain/Entry.h>
#include <Onyx/Domain/Wad.h>
#include <Onyx/Parsers/SceneNode.h>
#include <Onyx/Types/TypeId.h>
#include <Onyx/Vfs/IFile.h>

#include <memory>
#include <string>
#include <vector>

namespace Onyx::Gow2 {

/// The module-scoped TypeIds a real GOW2 tree carries, resolved by catalog
/// key. Construct one per build operation and pass it down rather than
/// caching in a static: Gow2Module::RegisterTypes() may not have run the
/// first time a handler is reached (module registration order is a
/// composition-root concern), and a stale "not found" would stick forever.
/// Any id the catalog does not know stays invalid, which never equals a real
/// entry's typeId -- so an unregistered module yields an empty scene rather
/// than a wrong one.
struct SceneTypes {
    SceneTypes();

    Onyx::Types::TypeId object, model, mesh, material, texture, script, animation, unknown;

    /// True when the module's types are present in the catalog. False means
    /// nothing will match and the caller should say so rather than silently
    /// building an empty scene.
    bool Valid() const;
};

/// Finds a node with this exact name and type that has children -- the real
/// definition behind a reference node. Mirrors the Go reference's
/// GetNodeByName.
const Onyx::Domain::AssetEntry* ResolveRef(const std::vector<Onyx::Domain::AssetEntry>& tree,
                                           const std::string& name,
                                           Onyx::Types::TypeId type);

/// Same, but for a node carrying a payload (size > 0) rather than children.
const Onyx::Domain::AssetEntry* ResolvePayload(const std::vector<Onyx::Domain::AssetEntry>& tree,
                                               const std::string& name,
                                               Onyx::Types::TypeId type);

/// Finds a Texture node by exact name anywhere in the tree.
const Onyx::Domain::AssetEntry* FindTexture(const std::vector<Onyx::Domain::AssetEntry>& nodes,
                                            const std::string& name,
                                            const SceneTypes& types);

/// The layer a GOW2 material actually draws, in the Go reference's priority
/// order: StrangeBlended (renderingMethod 3) > Usual (method 0 carrying a
/// texture) > the first layer. Null only for a material with no layers.
///
/// Public because it is the one place this port changed behaviour rather than
/// spelling: the pre-port code ran this selection twice (see the note on the
/// definition) and could land on a different layer the second time. Exposed so
/// gow2_material_test.cpp can pin the rule instead of leaving it unwitnessed.
const GOW2MaterialParser::MaterialLayer*
SelectMainLayer(const std::vector<GOW2MaterialParser::MaterialLayer>& layers);

/// Parses one GOW2 material entry into a v1.1 MaterialDesc, appends it to
/// `scene.materials`, and returns the name of the single texture it binds
/// (empty when it binds none).
///
/// A GOW2 material carries several layers and the game draws one of them.
/// That selection follows the Go reference's priority order -- StrangeBlended
/// (EnvMap) beats Usual (Normal with a texture), which beats the first layer
/// -- and the selected layer supplies the material's blend mode and blend
/// colour as well as its texture.
std::string StageMaterial(const Onyx::Domain::AssetEntry& matEntry,
                          const std::shared_ptr<Onyx::Vfs::IFile>& file,
                          Onyx::Parsers::SceneData& scene);

/// Decodes `textureName` into `scene.textures` and binds it as the Diffuse
/// role of `scene.materials[materialIndex]`. Does nothing when the name is
/// empty or does not resolve -- leaving the role absent, which is v1.1's
/// spelling of "this material has no diffuse map".
void BindDiffuse(const std::string& textureName,
                 size_t materialIndex,
                 Onyx::Domain::AssetContainer& wad,
                 const SceneTypes& types,
                 Onyx::Parsers::SceneData& scene);

} // namespace Onyx::Gow2
