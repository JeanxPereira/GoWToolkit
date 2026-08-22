#include "ui/WadBrowser.h"
#include <Onyx/App/UIHelpers.h>
#include "ui/RoleVisuals.h"   // GOWR role → color/icon (ColorForRole, IconForRole)
#include <Onyx/App/Widgets.h>
#include <Onyx/Modules/Workspace.h>
#include <Onyx/Services/AssetVisibility.h>
#include <Onyx/Services/Logger.h>
#include <Onyx/Api/ToolkitApi.h>
#include "core/WadTypes.h"
#include "core/domain/ContainerBridge.h"
#include "core/types/Gow2SceneBuild.h"
#include <Onyx/Fonts/SFSymbols.h>
#include "imgui.h"
#include <Onyx/App/ViewerRegistry.h>
#include <Onyx/Viewers/DocumentWindow.h>
#include "ui/viewers/MapViewer.h"
#include <functional>
#include <string>
#include <fstream>
#include "core/profiles/gowr/GowrTaxonomy.h"

// Onyx v1.1 removed AssetEntry::profileTag; role is reclassified on demand
// from the entry's own name + size via Gowr::Classify().
static Onyx::Gowr::WadEntryRole GetRole(const AssetEntry& e) {
    return Onyx::Gowr::Classify(e).role;
}

WadBrowser::WadBrowser(Onyx::Modules::Workspace& workspace)
    : m_workspace(workspace),
      m_openedSub(workspace.Events().On<Onyx::Modules::DocumentOpened>(
          [this](const Onyx::Modules::DocumentOpened&) { visible = true; })),
      m_closedSub(workspace.Events().On<Onyx::Modules::DocumentClosed>(
          [this](const Onyx::Modules::DocumentClosed& ev) {
              // Drop the bridge with the document so a reopened id cannot
              // inherit the previous document's entries.
              m_bridges.erase(ev.id);
              if (m_selDoc == ev.id) { m_selDoc = 0; m_selPath = {}; }
          })) {}

WadBrowser::~WadBrowser() = default;

Onyx::Domain::AssetContainer& WadBrowser::BridgeFor(Onyx::Modules::Document& doc) {
    auto it = m_bridges.find(doc.id);
    if (it != m_bridges.end()) return it->second;

    // Same projection AssetHarness::LoadContainer makes: resolve the backing
    // file through roots[0]'s own fileIndex rather than a hardcoded slot 0,
    // because a compressed GOWR wad addresses its decompressed buffer in a
    // later slot. Still not exact for a mounted GOW2 ISO, where different
    // entries can name different PART*.PAK files while AssetContainer has
    // only one fileSource -- that is AssetContainer's structural limit, noted
    // in the harness too.
    return m_bridges.emplace(doc.id, Onyx::Gow::MakeContainerBridge(doc)).first->second;
}

// ── Asset visibility ──────────────────────────────────────────────────────
// Determines whether an entry should appear in the browser tree.
// Delegates to the centralized AssetVisibility registry which handles both
// GOW2 (TypeId-based) and GOWR (role→TypeId mapping) in one code path.
// Users can toggle visibility per type via the Asset Filters panel.
static bool IsEntryVisible(const AssetEntry& entry) {
    return Onyx::Services::AssetVisibility::Get().IsVisible(entry.typeId);
}

void WadBrowser::Draw() {
    if (!visible) return;
    ImGui::Begin("WAD Browser", &visible);

    const auto& documents = m_workspace.Documents();
    if (documents.empty()) {
        ImGui::TextDisabled("No WAD loaded");
        ImGui::End();
        return;
    }

    // Resolved once per frame. A GOW2 tree carries both id families at once
    // -- legacy GameTypes::* where a handler resolved the tag, module-minted
    // ids where none did -- so the Model/Texture tests below have to accept
    // either. See Gow2SceneBuild.h.
    const Onyx::Gow2::SceneTypes types;

    static const char* kindNames[]           = {"All",   "Image",    "Mesh",     "Audio",
                                                "Video", "Material", "Animation"};
    static const Onyx::Domain::MediaKind kindValues[] = {
        Onyx::Domain::MediaKind::Unknown, // All
        Onyx::Domain::MediaKind::Image,   Onyx::Domain::MediaKind::Mesh,     Onyx::Domain::MediaKind::Audio,
        Onyx::Domain::MediaKind::Video,   Onyx::Domain::MediaKind::Material, Onyx::Domain::MediaKind::Animation};

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 130);
    ImGui::InputTextWithHint("##filter", "Filter entries...", m_filter, sizeof(m_filter));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::Combo("##kind_filter", &m_kindFilterIndex, kindNames, IM_ARRAYSIZE(kindNames));

    std::string filterLower(m_filter);
    for (auto& c : filterLower)
        c = (char)tolower(c);
    bool hasFilter = !filterLower.empty();

    bool hasKindFilter = (m_kindFilterIndex > 0);
    Onyx::Domain::MediaKind targetKind =
        hasKindFilter ? kindValues[m_kindFilterIndex] : Onyx::Domain::MediaKind::Unknown;

    std::function<bool(const AssetEntry&)> hasMatchingDescendant;
    hasMatchingDescendant = [&](const AssetEntry& entry) {
        if (entry.kind == targetKind) return true;
        for (const auto& child : entry.children) {
            if (hasMatchingDescendant(child)) return true;
        }
        return false;
    };

    ImGui::Separator();

    for (const auto& docPtr : documents) {
        if (!docPtr) continue;
        Onyx::Modules::Document& doc = *docPtr;

        // Workspace.h's thread rule: nothing but `ready` is safe to read on a
        // document whose parse may still be running on a worker.
        if (!doc.ready.load()) {
            ImGui::TextDisabled("%s  (parsing...)", doc.path.filename().string().c_str());
            continue;
        }

        Onyx::Domain::AssetContainer& wad = BridgeFor(doc);

        ImGui::PushID((int)doc.id);
        // Two square IconButtons stack to the right of the WAD header.
        const float btnSize    = ImGui::GetFrameHeight();
        const float spacing    = ImGui::GetStyle().ItemSpacing.x;
        const float buttonsWidth = btnSize * 2.0f + spacing;
        const float windowWidth  = ImGui::GetContentRegionAvail().x;

        bool wadOpen = ImGui::TreeNodeEx(wad.filename.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(windowWidth - buttonsWidth);
        ImGui::SetNextItemAllowOverlap();
        {
            Onyx::App::Widgets::IconButtonOpts opts;
            opts.tooltip = "Load Entire Map/Level";
            if (Onyx::App::Widgets::IconButton("wad_map", ICON_SF_MAP_FILL, opts)) {
                auto viewer = std::make_shared<Onyx::MapViewer>(wad.filename, wad);
                Onyx::Api::Documents().AddTab(viewer);
            }
        }

        ImGui::SameLine();
        ImGui::SetNextItemAllowOverlap();
        {
            Onyx::App::Widgets::IconButtonOpts opts;
            opts.tooltip = "Close WAD";
            if (Onyx::App::Widgets::IconButton("wad_close", ICON_SF_XMARK, opts)) {
                if (wadOpen) ImGui::TreePop();
                ImGui::PopID();
                // Close posts DocumentClosed, which drops the bridge above.
                m_workspace.Close(doc.id);
                break;
            }
        }

        if (wadOpen) {
            // `path` is extended with each child index visited, so a click
            // always carries the exact NodePath that reached the clicked
            // node -- positional addressing survives duplicate sibling names
            // where a name-based key would not.
            Onyx::Modules::NodePath path;
            int entryIdx = 0;

            std::function<void(AssetEntry&, uint32_t, int&)> renderEntryTree;
            renderEntryTree = [&](AssetEntry& entry, uint32_t index, int& idx) {
                // Filter: check name match
                if (hasFilter) {
                    std::string nameLower = entry.name;
                    for (auto& c : nameLower)
                        c = (char)tolower(c);
                    std::string typeLower = TypeName(entry.typeId);
                    for (auto& c : typeLower)
                        c = (char)tolower(c);

                    bool matchesFilter = (nameLower.find(filterLower) != std::string::npos ||
                                          typeLower.find(filterLower) != std::string::npos);
                    if (!matchesFilter && entry.children.empty()) return;
                }

                // Kind filter check: respects hierarchy
                if (hasKindFilter) {
                    if (entry.kind != targetKind && !hasMatchingDescendant(entry)) {
                        return;
                    }
                }

                // ── Asset visibility filter (GOW2 + GOWR) ───────────
                if (!IsEntryVisible(entry)) {
                    return;
                }

                path.indices.push_back(index);
                struct PathPop {
                    Onyx::Modules::NodePath& p;
                    ~PathPop() { p.indices.pop_back(); }
                } pathPop{path};

                const bool isSelected =
                    (m_selDoc == doc.id && m_selPath.indices == path.indices);

                ImGui::PushID(idx);
                bool has_children = !entry.children.empty();

                // ── Flags ────────────────────────────────────────────
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
                if (!has_children) {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                } else {
                    flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
                }
                if (hasFilter) flags |= ImGuiTreeNodeFlags_DefaultOpen;
                if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

                // ── Icon + color (prefer role-based for GOWR entries) ────
                const char* icon;
                ImVec4 color;
                auto role = GetRole(entry);
                if (role != Onyx::Gowr::WadEntryRole::Unknown) {
                    icon  = IconForRole(role);
                    color = ColorForRole(role);
                } else {
                    icon  = IconForType(entry.typeId);
                    color = ColorForType(entry.typeId);
                }

                // Use displayName if set, otherwise fall back to name
                const std::string& label_name =
                    entry.displayName.empty() ? entry.name : entry.displayName;

                // ── TreeNode with formatted label ────────────────────
                bool node_open = Onyx::App::Widgets::ColoredTreeNode("", label_name.c_str(), icon, color, flags, isSelected);

                // ── Selection (single click) — announced on the bus ──
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    m_selDoc  = doc.id;
                    m_selPath = path;
                    m_workspace.Events().Post(
                        Onyx::Modules::SelectionChanged{doc.id, path});
                }

                // ── Double-click action ────────────────────────────────
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (wad.fileSource) {
                        auto viewer = Onyx::Api::Viewers().Open(entry, wad);
                        if (viewer) Onyx::Api::Documents().AddTab(viewer);
                    }
                }

                // ── Tooltip on hover ─────────────────────────────────
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Type: %s", TypeName(entry.typeId));
                    ImGui::Text("Offset: 0x%08llX",
                                (unsigned long long)entry.source.offset);
                    ImGui::Text("Size: %s", FormatBytes(entry.source.size).c_str());
                    ImGui::EndTooltip();
                }

                // ── Right-click Context Menu ─────────────────────────
                if (ImGui::BeginPopupContextItem()) {
                    ImGui::TextDisabled("%s", entry.name.c_str());
                    ImGui::TextDisabled("%s  |  %s", TypeName(entry.typeId),
                                        FormatBytes(entry.source.size).c_str());
                    ImGui::Separator();

                    if (wad.fileSource &&
                        Onyx::Api::Viewers().CanHandle(entry.typeId)) {
                        auto title = std::string(ICON_SF_FOLDER_FILL) + "  Open";
                        if (ImGui::MenuItem(title.c_str())) {
                            auto viewer = Onyx::Api::Viewers().Open(entry, wad);
                            if (viewer) Onyx::Api::Documents().AddTab(viewer);
                        }
                    }

                    // Type-specific extras: "View All Textures" for MDL with TXR children
                    if (types.model.Known() && types.model.Matches(entry.typeId) &&
                        has_children && wad.fileSource) {
                        int txrCount = 0;
                        for (const auto& c : entry.children) {
                            if (types.texture.Matches(c.typeId)) txrCount++;
                        }
                        if (txrCount > 0) {
                            char menuLabel[64];
                            snprintf(menuLabel, sizeof(menuLabel),
                                     ICON_SF_PHOTO " View All Textures (%d)", txrCount);
                            if (ImGui::MenuItem(menuLabel)) {
                                for (const auto& c : entry.children) {
                                    if (types.texture.Matches(c.typeId)) {
                                        auto viewer = Onyx::Api::Viewers().Open(c, wad);
                                        if (viewer) Onyx::Api::Documents().AddTab(viewer);
                                    }
                                }
                            }
                        }
                    }

                    if (ImGui::MenuItem(ICON_SF_DOCUMENT_ON_DOCUMENT " Copy Name")) {
                        ImGui::SetClipboardText(entry.name.c_str());
                    }

                    if (wad.fileSource &&
                        ImGui::MenuItem(ICON_SF_SQUARE_AND_ARROW_DOWN " Extract File")) {
                        std::string savePath = SystemSaveFileDialog(entry.name);
                        if (!savePath.empty()) {
                            std::vector<uint8_t> dumpData(entry.source.size);
                            wad.fileSource->Seek(entry.source.offset, 0);
                            wad.fileSource->Read(dumpData.data(), entry.source.size);
                            if (!dumpData.empty()) {
                                std::ofstream out(savePath, std::ios::binary);
                                if (out.is_open()) {
                                    out.write(reinterpret_cast<const char*>(dumpData.data()),
                                              dumpData.size());
                                    out.close();
                                    ONYX_LOGF_INFO("Extracted %s to %s", entry.name.c_str(),
                                             savePath.c_str());
                                } else {
                                    ONYX_LOGF_ERR("Failed to open path for writing: %s",
                                            savePath.c_str());
                                }
                            }
                        }
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopID();
                idx++;

                // Render children
                if (node_open && has_children) {
                    for (uint32_t ci = 0; ci < entry.children.size(); ++ci) {
                        renderEntryTree(entry.children[ci], ci, idx);
                    }
                    ImGui::TreePop();
                }
            };

            for (uint32_t ri = 0; ri < wad.entries.size(); ++ri) {
                renderEntryTree(wad.entries[ri], ri, entryIdx);
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::End();
}
