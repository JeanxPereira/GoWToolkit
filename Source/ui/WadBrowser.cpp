#include "ui/WadBrowser.h"
#include <Onyx/App/UIHelpers.h>
#include "ui/RoleVisuals.h"   // GOWR role → color/icon (ColorForRole, IconForRole)
#include <Onyx/App/Widgets.h>
#include <Onyx/Services/AssetDatabase.h>
#include <Onyx/Services/AssetVisibility.h>
#include <Onyx/Services/Events.h>
#include <Onyx/Services/Logger.h>
#include <Onyx/Api/ToolkitApi.h>
#include "core/WadTypes.h"
#include "core/types/GameTypes.h"
#include <Onyx/Fonts/SFSymbols.h>
#include "imgui.h"
#include <Onyx/App/ViewerRegistry.h>
#include <Onyx/Viewers/DocumentWindow.h>
#include "ui/viewers/MapViewer.h"
#include <functional>
#include <string>
#include <fstream>
#include "core/profiles/gowr/GowrProfileTag.h"

static Onyx::Gowr::WadEntryRole GetRole(const AssetEntry& e) {
    if (auto* t = e.profileTag.As<Onyx::Gowr::GowrProfileTag>()) {
        return t->role;
    }
    return Onyx::Gowr::WadEntryRole::Unknown;
}

WadBrowser::WadBrowser() {
    EventWadOpened::subscribe(this, [this](AssetContainer*) { visible = true; });
}

WadBrowser::~WadBrowser() {
    EventWadOpened::unsubscribe(this);
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

    auto& db = Onyx::Api::Database();

    if (db.wads.empty()) {
        ImGui::TextDisabled("No WAD loaded");
        ImGui::End();
        return;
    }

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

    for (size_t wadIdx = 0; wadIdx < db.wads.size(); ++wadIdx) {
        auto& wad = db.wads[wadIdx];
        ImGui::PushID((int)wadIdx);
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
                // CloseWad posts EventWadClosed itself, before it erases.
                db.CloseWad(wadIdx);
                break;
            }
        }

        if (wadOpen) {
            int entryIdx = 0;

            // Recursive lambda for rendering tree
            std::function<void(AssetEntry&, int&)> renderEntryTree;
            renderEntryTree = [&](AssetEntry& entry, int& idx) {
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
                // Delegates to AssetVisibility registry. Users can toggle
                // types on/off via the Asset Filters panel.
                if (!IsEntryVisible(entry)) {
                    return;
                }

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
                if (Onyx::Api::GetSelected() == &entry) flags |= ImGuiTreeNodeFlags_Selected;

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
                bool isSelected = (Onyx::Api::GetSelected() == &entry);
                bool node_open = Onyx::App::Widgets::ColoredTreeNode("", label_name.c_str(), icon, color, flags, isSelected);

                // ── Selection (single click) — via Api::SetSelected ──
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    db.EnsureNodeData(&entry, wad);
                    Onyx::Api::SetSelected(&entry, &wad);
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
                    ImGui::Text("Offset: 0x%08X", entry.source.offset);
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
                    if (entry.typeId == Onyx::GameTypes::Model && has_children && wad.fileSource) {
                        int txrCount = 0;
                        for (const auto& c : entry.children) {
                            if (c.typeId == Onyx::GameTypes::Texture) txrCount++;
                        }
                        if (txrCount > 0) {
                            char menuLabel[64];
                            snprintf(menuLabel, sizeof(menuLabel),
                                     ICON_SF_PHOTO " View All Textures (%d)", txrCount);
                            if (ImGui::MenuItem(menuLabel)) {
                                for (const auto& c : entry.children) {
                                    if (c.typeId == Onyx::GameTypes::Texture) {
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
                    for (auto& child : entry.children) {
                        renderEntryTree(child, idx);
                    }
                    ImGui::TreePop();
                }
            };

            for (auto& entry : wad.entries) {
                renderEntryTree(entry, entryIdx);
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::End();
}
