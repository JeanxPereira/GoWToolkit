#include "ui/WadBrowser.h"
#include "UIHelpers.h"
#include "ui/Widgets.h"
#include "core/AssetDatabase.h"
#include "core/AssetVisibility.h"
#include "core/Events.h"
#include "core/Logger.h"
#include "core/ToolkitApi.h"
#include "core/WadTypes.h"
#include "fonts/SFSymbols.h"
#include "imgui.h"
#include "ui/ViewerRegistry.h"
#include "ui/viewers/DocumentWindow.h"
#include "ui/viewers/MapViewer.h"
#include <functional>
#include <string>
#include <fstream>
#include "core/profiles/gowr/GowrProfileTag.h"

static GOW::Gowr::WadEntryRole GetRole(const ParsedEntry& e) {
    if (auto* t = e.profileTag.As<GOW::Gowr::GowrProfileTag>()) {
        return t->role;
    }
    return GOW::Gowr::WadEntryRole::Unknown;
}

WadBrowser::WadBrowser() {
    EventWadOpened::subscribe(this, [this](OpenWad*) { visible = true; });
}

WadBrowser::~WadBrowser() {
    EventWadOpened::unsubscribe(this);
}

// ── Asset visibility ──────────────────────────────────────────────────────
// Determines whether an entry should appear in the browser tree.
// Delegates to the centralized AssetVisibility registry which handles both
// GOW2 (TypeId-based) and GOWR (role→TypeId mapping) in one code path.
// Users can toggle visibility per type via the Asset Filters panel.
static GOW::GameVersion DetectGameVersion(const ParsedEntry& e) {
    // GOWR entries have a classified role via GowrProfileTag
    if (auto* t = e.profileTag.As<GOW::Gowr::GowrProfileTag>()) {
        if (t->role != GOW::Gowr::WadEntryRole::Unknown)
            return GOW::GameVersion::GOWR;
    }
    return GOW::GameVersion::GOW2;
}

static bool IsEntryVisible(const ParsedEntry& entry) {
    auto ver = DetectGameVersion(entry);
    return GOW::AssetVisibility::Get().IsVisible(ver, entry.typeId);
}


void WadBrowser::Draw() {
    if (!visible) return;
    ImGui::Begin("WAD Browser", &visible);

    auto& db = GOW::Api::Database();

    if (db.wads.empty()) {
        ImGui::TextDisabled("No WAD loaded");
        ImGui::End();
        return;
    }

    static const char* kindNames[]           = {"All",   "Image",    "Mesh",     "Audio",
                                                "Video", "Material", "Animation"};
    static const GOW::MediaKind kindValues[] = {
        GOW::MediaKind::Unknown, // All
        GOW::MediaKind::Image,   GOW::MediaKind::Mesh,     GOW::MediaKind::Audio,
        GOW::MediaKind::Video,   GOW::MediaKind::Material, GOW::MediaKind::Animation};

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
    GOW::MediaKind targetKind =
        hasKindFilter ? kindValues[m_kindFilterIndex] : GOW::MediaKind::Unknown;

    std::function<bool(const ParsedEntry&)> hasMatchingDescendant;
    hasMatchingDescendant = [&](const ParsedEntry& entry) {
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
            GOW::UI::Widgets::IconButtonOpts opts;
            opts.tooltip = "Load Entire Map/Level";
            if (GOW::UI::Widgets::IconButton("wad_map", ICON_SF_MAP_FILL, opts)) {
                auto viewer = std::make_shared<GOW::MapViewer>(wad.filename, wad);
                GOW::Api::Documents().AddTab(viewer);
            }
        }

        ImGui::SameLine();
        ImGui::SetNextItemAllowOverlap();
        {
            GOW::UI::Widgets::IconButtonOpts opts;
            opts.tooltip = "Close WAD";
            if (GOW::UI::Widgets::IconButton("wad_close", ICON_SF_XMARK, opts)) {
                if (wadOpen) ImGui::TreePop();
                ImGui::PopID();
                EventWadClosed::post(wadIdx);
                db.CloseWad(wadIdx);
                break;
            }
        }

        if (wadOpen) {
            int entryIdx = 0;

            // Recursive lambda for rendering tree
            std::function<void(ParsedEntry&, int&)> renderEntryTree;
            renderEntryTree = [&](ParsedEntry& entry, int& idx) {
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
                if (GOW::Api::GetSelected() == &entry) flags |= ImGuiTreeNodeFlags_Selected;

                // ── Icon + color (prefer role-based for GOWR entries) ────
                const char* icon;
                ImVec4 color;
                auto role = GetRole(entry);
                if (role != GOW::Gowr::WadEntryRole::Unknown) {
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
                bool isSelected = (GOW::Api::GetSelected() == &entry);
                bool node_open = GOW::UI::Widgets::ColoredTreeNode("", label_name.c_str(), icon, color, flags, isSelected);

                // ── Selection (single click) — via Api::SetSelected ──
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    db.EnsureNodeData(&entry, wad);
                    GOW::Api::SetSelected(&entry, &wad);
                }

                // ── Double-click action ────────────────────────────────
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (wad.fileSource) {
                        auto viewer = GOW::Api::Viewers().Open(entry, wad);
                        if (viewer) GOW::Api::Documents().AddTab(viewer);
                    }
                }

                // ── Tooltip on hover ─────────────────────────────────
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Type: %s", TypeName(entry.typeId));
                    ImGui::Text("Offset: 0x%08X", entry.offset);
                    ImGui::Text("Size: %s", FormatBytes(entry.size).c_str());
                    ImGui::EndTooltip();
                }

                // ── Right-click Context Menu ─────────────────────────
                if (ImGui::BeginPopupContextItem()) {
                    ImGui::TextDisabled("%s", entry.name.c_str());
                    ImGui::TextDisabled("%s  |  %s", TypeName(entry.typeId),
                                        FormatBytes(entry.size).c_str());
                    ImGui::Separator();

                    if (wad.fileSource &&
                        GOW::Api::Viewers().CanHandle(entry.typeId)) {
                        auto title = std::string(ICON_SF_FOLDER_FILL) + "  Open";
                        if (ImGui::MenuItem(title.c_str())) {
                            auto viewer = GOW::Api::Viewers().Open(entry, wad);
                            if (viewer) GOW::Api::Documents().AddTab(viewer);
                        }
                    }

                    // Type-specific extras: "View All Textures" for MDL with TXR children
                    if (entry.typeId == GOW::TypeId::Model && has_children && wad.fileSource) {
                        int txrCount = 0;
                        for (const auto& c : entry.children) {
                            if (c.typeId == GOW::TypeId::Texture) txrCount++;
                        }
                        if (txrCount > 0) {
                            char menuLabel[64];
                            snprintf(menuLabel, sizeof(menuLabel),
                                     ICON_SF_PHOTO " View All Textures (%d)", txrCount);
                            if (ImGui::MenuItem(menuLabel)) {
                                for (const auto& c : entry.children) {
                                    if (c.typeId == GOW::TypeId::Texture) {
                                        auto viewer = GOW::Api::Viewers().Open(c, wad);
                                        if (viewer) GOW::Api::Documents().AddTab(viewer);
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
                            std::vector<uint8_t> dumpData(entry.size);
                            wad.fileSource->Seek(entry.offset, 0);
                            wad.fileSource->Read(dumpData.data(), entry.size);
                            if (!dumpData.empty()) {
                                std::ofstream out(savePath, std::ios::binary);
                                if (out.is_open()) {
                                    out.write(reinterpret_cast<const char*>(dumpData.data()),
                                              dumpData.size());
                                    out.close();
                                    LOG_INFO("Extracted %s to %s", entry.name.c_str(),
                                             savePath.c_str());
                                } else {
                                    LOG_ERR("Failed to open path for writing: %s",
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