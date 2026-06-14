// Instance handler Ã¢â‚¬â€ GOW1/GOW2 game object instances (goarcher00, gohero00, etc.)
// Magic: 0x00030001 (GOW2), 0x00020001 (GOW1)
//
// Resolution follows the Go project (god_of_war_browser):
//   GOW2: Object is SubGroupNodes[0] (first child in tree)
//   GOW1: Object is looked up by name stored in Instance binary data
// Then delegates to ObjectHandler.

#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Types/ITypeHandler.h>
#include "core/types/GameTypes.h"
#include "core/WadTypes.h"
#include <Onyx/Services/Logger.h>
#include "core/parsers/gow2/InstanceParser.h"
#include <Onyx/Viewers/Viewport3D.h>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Onyx/Fonts/SFSymbols.h>

namespace {

// Ã¢â€â‚¬Ã¢â€â‚¬ Helpers Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

// Find a AssetEntry by exact name and TypeId in the WAD tree.
// Mirrors Go's GetNodeByName: searches backwards from the instance's position.
// We search the full tree since we don't have positional ordering.
static const AssetEntry* FindEntryByNameAndType(
    const std::vector<AssetEntry>& tree,
    const std::string& name, Onyx::Types::TypeId type)
{
    for (const auto& n : tree) {
        if (n.name == name && n.typeId == type)
            return &n;
        if (auto found = FindEntryByNameAndType(n.children, name, type))
            return found;
    }
    return nullptr;
}

// Find by name only (any type that has children or payload)
static const AssetEntry* FindEntryByName(
    const std::vector<AssetEntry>& tree,
    const std::string& name)
{
    for (const auto& n : tree) {
        if (n.name == name && (!n.children.empty() || n.size > 0))
            return &n;
        if (auto found = FindEntryByName(n.children, name))
            return found;
    }
    return nullptr;
}

// Ã¢â€â‚¬Ã¢â€â‚¬ InstanceHandler Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

class InstanceHandler : public Onyx::Types::ITypeHandler {
public:
    Onyx::Types::TypeId  GetId()    const override { return Onyx::GameTypes::Instance; }
    const char*  GetName()  const override { return "Instance"; }
    uint32_t     GetMagic() const override { return 0x00030001; }
    const char*  GetIcon()  const override { return ICON_SF_PERSON_FILL; }
    Color4f      GetColor() const override { return {1.0f, 0.7f, 0.7f, 1.0f}; }

    std::unique_ptr<Onyx::Parsers::SceneData> BuildSceneData(const AssetEntry& entry, AssetContainer& wad) override {
        // 1. Parse instance transform
        auto instData = Onyx::GOW2InstanceParser::Parse(entry, wad.fileSource);
        if (!instData) {
            LOG_WARN("[InstanceHandler] Failed to parse instance data for '%s'", entry.name.c_str());
            return nullptr;
        }

        // 2. Find the Object/Model this instance references.
        //
        // GOW2 (magic 0x00030001, size 0x68):
        //   The Object is the first child (SubGroupNodes[0]).
        //   See: god_of_war_browser/pack/wad/inst/gow2.go:56
        //     oNId := wrsrc.Node.SubGroupNodes[0]

        const AssetEntry* objEntry = nullptr;

        // Ã¢â€â‚¬Ã¢â€â‚¬ GOW2 path: find child Object/Model Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬
        {
            const AssetEntry* sourceEntry = &entry;

            // If this instance is a zero-sized reference (no children),
            // try to find the original definition with children
            if (entry.children.empty() && entry.size > 0 && !entry.name.empty()) {
                std::function<const AssetEntry*(const std::vector<AssetEntry>&)> findOriginal =
                    [&](const std::vector<AssetEntry>& list) -> const AssetEntry* {
                    for (const auto& n : list) {
                        if (n.name == entry.name && !n.children.empty()) {
                            return &n;
                        }
                        if (auto found = findOriginal(n.children)) {
                            return found;
                        }
                    }
                    return nullptr;
                };
                if (auto orig = findOriginal(wad.entries)) {
                    sourceEntry = orig;
                }
            }

            // Search children for Object, then Model
            for (const auto& child : sourceEntry->children) {
                if (child.typeId == Onyx::GameTypes::Object) {
                    objEntry = &child;
                    break;
                }
            }
            if (!objEntry) {
                for (const auto& child : sourceEntry->children) {
                    if (child.typeId == Onyx::GameTypes::Model) {
                        objEntry = &child;
                        break;
                    }
                }
            }
        }

        if (!objEntry) {
            LOG_WARN("[InstanceHandler] No Object or Model found for '%s'", entry.name.c_str());
            return nullptr;
        }

        // 3. Delegate scene building to the child handler
        auto* handler = Onyx::Types::TypeRegistry::Get().Resolve(objEntry->typeId);
        if (!handler) {
            LOG_WARN("[InstanceHandler] No handler for typeId=%d in '%s'", (int)objEntry->typeId.value, entry.name.c_str());
            return nullptr;
        }

        auto scene = handler->BuildSceneData(*objEntry, wad);
        if (!scene) {
            LOG_WARN("[InstanceHandler] handler->BuildSceneData returned null for '%s' Ã¢â€ â€™ '%s'",
                     entry.name.c_str(), objEntry->name.c_str());
            return nullptr;
        }

        // 4. Sky detection Ã¢â‚¬â€ three sources, any one triggers sky routing:
        //    a) ObjectHandler/ProcessModel sets part.isSky when SCR_Sky child
        //       lives on a Model under an Object (matches god_of_war_browser).
        //    b) ModelHandler sets scene->isSky when SCR_Sky child lives on the
        //       Model directly (instance child is a Model, not an Object).
        //    c) InstanceParser sets instData->isSky when the instance name
        //       contains "sky" (fallback heuristic).
        // Promote any signal to BOTH scene-level and per-part flags so the
        // SceneRenderer (batch.isSky = part.isSky) can route via RenderSky.
        bool partSky = false;
        for (const auto& part : scene->meshParts) {
            if (part.isSky) { partSky = true; break; }
        }
        if (instData->isSky || partSky || scene->isSky) {
            scene->isSky = true;
            for (auto& part : scene->meshParts) {
                part.isSky = true;
            }
            LOG_INFO("[InstanceHandler] Marking scene as sky for instance '%s' (nameMatch=%d, partFlag=%d, sceneFlag=%d)",
                     entry.name.c_str(), instData->isSky, partSky, scene->isSky);
        }

        // 5. Instance transform Ã¢â‚¬â€ GOW2: do NOT apply.
        //
        // Reference: god_of_war_browser/web/data/static/js/BrowserWad.js:1365
        //   if (inst.IsGow2) {
        //     // instNode.setLocalMatrix(instMat);   Ã¢â€ Â COMMENTED OUT
        //   }
        // Joint world transforms (renderMat from Matrixes1 chain) already place
        // GOW2 geometry in world space. Applying inst.Position on top double-
        // counts position, producing wrong placement in CXT map merge and
        // doubled coordinates in the single-instance viewer.
        //
        // We keep instanceTransform on the scene so callers can introspect the
        // raw matrix if needed (debug/UI), but it is identity for render
        // purposes Ã¢â‚¬â€ SceneRenderer's flipZ scale still applies as configured.
        scene->instanceTransform = glm::mat4(1.0f);

        return scene;
    }

    std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry& entry, AssetContainer& wad) override {
        auto vp = std::make_shared<Onyx::Viewers::Viewport3D>(entry.name);
        if (auto scene = BuildSceneData(entry, wad)) {
            vp->LoadScene(std::move(scene));
        }
        return vp;
    }
};

} // anonymous namespace

REGISTER_TYPE(GOW2, InstanceHandler);

