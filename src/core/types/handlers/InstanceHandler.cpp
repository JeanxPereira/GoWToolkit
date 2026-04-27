// Instance handler — GOW1/GOW2 game object instances (goarcher00, gohero00, etc.)
// Magic: 0x00030001 (GOW2), 0x00020001 (GOW1)
//
// Resolution follows the Go project (god_of_war_browser):
//   GOW2: Object is SubGroupNodes[0] (first child in tree)
//   GOW1: Object is looked up by name stored in Instance binary data
// Then delegates to ObjectHandler.

#include "core/types/TypeRegistry.h"
#include "core/types/ITypeHandler.h"
#include "core/WadTypes.h"
#include "core/Logger.h"
#include "core/parsers/gow2/InstanceParser.h"
#include "ui/viewers/Viewport3D.h"
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "fonts/SFSymbols.h"

namespace {

// ── Helpers ────────────────────────────────────────────────────────────────

// Find a ParsedEntry by exact name and TypeId in the WAD tree.
// Mirrors Go's GetNodeByName: searches backwards from the instance's position.
// We search the full tree since we don't have positional ordering.
static const ParsedEntry* FindEntryByNameAndType(
    const std::vector<ParsedEntry>& tree,
    const std::string& name, GOW::TypeId type)
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
static const ParsedEntry* FindEntryByName(
    const std::vector<ParsedEntry>& tree,
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

// ── InstanceHandler ────────────────────────────────────────────────────────

class InstanceHandler : public GOW::ITypeHandler {
public:
    GOW::TypeId  GetId()    const override { return GOW::TypeId::Instance; }
    const char*  GetName()  const override { return "Instance"; }
    uint32_t     GetMagic() const override { return 0x00030001; }
    const char*  GetIcon()  const override { return ICON_SF_PERSON_FILL; }
    Color4f      GetColor() const override { return {1.0f, 0.7f, 0.7f, 1.0f}; }

    std::unique_ptr<GOW::SceneData> BuildSceneData(const ParsedEntry& entry, OpenWad& wad) override {
        // 1. Parse instance transform
        auto instData = GOW::GOW2InstanceParser::Parse(entry, wad.fileSource);
        if (!instData) {
            LOG_WARN("[InstanceHandler] Failed to parse instance data for '%s'", entry.name.c_str());
            return nullptr;
        }

        // 2. Find the Object/Model this instance references.
        //
        // Two strategies depending on game version:
        //
        // GOW1 (magic 0x00020001, size 0x5C):
        //   The Instance binary data contains the Object name at [0x04:0x1C].
        //   The Object is a SIBLING in the WAD tree, found by name lookup.
        //   See: god_of_war_browser/pack/wad/inst/gameobject.go:59
        //     objNode := wrsrc.Wad.GetNodeByName(inst.Object, wrsrc.Node.Id-1, false)
        //
        // GOW2 (magic 0x00030001, size 0x68):
        //   The Object is the first child (SubGroupNodes[0]).
        //   See: god_of_war_browser/pack/wad/inst/gow2.go:56
        //     oNId := wrsrc.Node.SubGroupNodes[0]

        const ParsedEntry* objEntry = nullptr;

        // ── GOW1 path: resolve Object by name ──────────────────────────
        if (!instData->objectName.empty()) {
            LOG_INFO("[InstanceHandler] GOW1 path: resolving object '%s' for instance '%s'",
                     instData->objectName.c_str(), entry.name.c_str());

            // Try Object type first, then fall back to any matching entry
            objEntry = FindEntryByNameAndType(wad.entries, instData->objectName, GOW::TypeId::Object);
            if (!objEntry) {
                objEntry = FindEntryByName(wad.entries, instData->objectName);
            }
            if (!objEntry) {
                LOG_WARN("[InstanceHandler] GOW1: Could not find object '%s' for instance '%s'",
                         instData->objectName.c_str(), entry.name.c_str());
                return nullptr;
            }
        }
        // ── GOW2 path: find child Object/Model ─────────────────────────
        else {
            const ParsedEntry* sourceEntry = &entry;

            // If this instance is a zero-sized reference (no children),
            // try to find the original definition with children
            if (entry.children.empty() && entry.size > 0 && !entry.name.empty()) {
                std::function<const ParsedEntry*(const std::vector<ParsedEntry>&)> findOriginal =
                    [&](const std::vector<ParsedEntry>& list) -> const ParsedEntry* {
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
                if (child.typeId == GOW::TypeId::Object) {
                    objEntry = &child;
                    break;
                }
            }
            if (!objEntry) {
                for (const auto& child : sourceEntry->children) {
                    if (child.typeId == GOW::TypeId::Model) {
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
        auto* handler = GOW::TypeRegistry::Get().Resolve(objEntry->typeId);
        if (!handler) {
            LOG_WARN("[InstanceHandler] No handler for typeId=%d in '%s'", (int)objEntry->typeId, entry.name.c_str());
            return nullptr;
        }

        auto scene = handler->BuildSceneData(*objEntry, wad);
        if (!scene) {
            LOG_WARN("[InstanceHandler] handler->BuildSceneData returned null for '%s' → '%s'",
                     entry.name.c_str(), objEntry->name.c_str());
            return nullptr;
        }

        // 4. Apply sky flag from instance data
        if (instData->isSky) {
            scene->isSky = true;
            for (auto& part : scene->meshParts) {
                part.isSky = true;
            }
            LOG_INFO("[InstanceHandler] Marking scene as sky for instance '%s'", entry.name.c_str());
        }

        // 5. Apply transform
        // For skinned dynamic models (gohero00), we pass the transform down to the hardware
        // to avoid destroying bind-pose vertices before the bone skinning takes place.
        // For static instances (like CXT levels), we MUST pre-multiply the vertices so that
        // WAD map viewer can merge thousands of statically placed parts into a single SceneData.
        if (scene->HasSkeleton()) {
            scene->instanceTransform = glm::make_mat4(instData->transformMatrix);
        } else {
            glm::mat4 m = glm::make_mat4(instData->transformMatrix);
            glm::mat3 m3(m);

            for (auto& part : scene->meshParts) {
                for (auto& v : part.vertices) {
                    glm::vec4 pos(v.position[0], v.position[1], v.position[2], 1.0f);
                    pos = m * pos;
                    v.position[0] = pos.x;
                    v.position[1] = pos.y;
                    v.position[2] = pos.z;

                    glm::vec3 n3(v.normal[0], v.normal[1], v.normal[2]);
                    n3 = m3 * n3;
                    v.normal[0] = n3.x;
                    v.normal[1] = n3.y;
                    v.normal[2] = n3.z;
                }
            }
        }

        return scene;
    }

    std::shared_ptr<GOW::IDocumentContent> CreateViewer(const ParsedEntry& entry, OpenWad& wad) override {
        auto vp = std::make_shared<GOW::Viewport3D>(entry.name);
        if (auto scene = BuildSceneData(entry, wad)) {
            vp->LoadScene(std::move(scene));
        }
        return vp;
    }
};

} // anonymous namespace

REGISTER_TYPE(GOW2, InstanceHandler);
REGISTER_TYPE(GOW1, InstanceHandler);
