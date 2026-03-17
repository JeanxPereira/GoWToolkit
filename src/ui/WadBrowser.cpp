#include "ui/WadBrowser.h"
#include "ui/AppContext.h"
#include "core/AssetDatabase.h"
#include "ui/ViewerRegistry.h"
#include "ui/viewers/DocumentWindow.h"
#include "UIHelpers.h"
#include "imgui.h"
#include "core/Logger.h"
#include <string>
#include <functional>

void WadBrowser::draw(AppContext& ctx) {
    if (!visible) return;
    ImGui::Begin("WAD Browser", &visible);

    auto& db = ctx.db;

    if (db.wads.empty()) {
        ImGui::TextDisabled("No WAD loaded");
        ImGui::End();
        return;
    }

    // Filter
    static char filterBuf[128] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##filter", "Filter entries...", filterBuf, sizeof(filterBuf));
    std::string filterLower(filterBuf);
    for (auto& c : filterLower) c = (char)tolower(c);
    bool hasFilter = !filterLower.empty();

    ImGui::Separator();

    for (size_t wadIdx = 0; wadIdx < db.wads.size(); ++wadIdx) {
        auto& wad = db.wads[wadIdx];
        ImGui::PushID((int)wadIdx);

        bool wadOpen = ImGui::TreeNodeEx(wad.filename.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (wadOpen) {
            int entryIdx = 0;

            // Recursive lambda for rendering tree
            std::function<void(ParsedEntry&, int&)> renderEntryTree;
            renderEntryTree = [&](ParsedEntry& entry, int& idx) {
                // Filter: check name match
                if (hasFilter) {
                    std::string nameLower = entry.name;
                    for (auto& c : nameLower) c = (char)tolower(c);
                    std::string typeLower = entry.schemaType;
                    for (auto& c : typeLower) c = (char)tolower(c);

                    bool matchesFilter = (nameLower.find(filterLower) != std::string::npos ||
                                         typeLower.find(filterLower) != std::string::npos);
                    if (!matchesFilter && entry.children.empty()) return;
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
                if (ctx.selected == &entry) flags |= ImGuiTreeNodeFlags_Selected;

                // ── Icon + color ─────────────────────────────────────
                const char* icon = IconForType(entry.schemaType);
                ImVec4 color = ColorForType(entry.schemaType);

                // ── TreeNode with formatted label ────────────────────
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                char label[256];
                snprintf(label, sizeof(label), "%s  %s", icon, entry.name.c_str());
                bool node_open = ImGui::TreeNodeEx(label, flags);
                ImGui::PopStyleColor();

                // ── Selection (single click) ─────────────────────────
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    ctx.selected = &entry;
                    db.EnsureNodeData(ctx.selected, wad);
                }

                // ── Double-click action ────────────────────────────────
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (wad.fileSource) {
                        auto viewer = ctx.viewerRegistry.Open(entry, wad);
                        if (viewer)
                            ctx.documentWindow.AddTab(viewer);
                    }
                }

                // ── Tooltip on hover ─────────────────────────────────
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Type: %s", entry.schemaType.c_str());
                    ImGui::Text("Offset: 0x%08X", entry.offset);
                    ImGui::Text("Size: %s", FormatBytes(entry.size).c_str());
                    ImGui::EndTooltip();
                }

                // ── Right-click Context Menu ─────────────────────────
                if (ImGui::BeginPopupContextItem()) {
                    ImGui::TextDisabled("%s", entry.name.c_str());
                    ImGui::TextDisabled("%s  |  %s", entry.schemaType.c_str(), FormatBytes(entry.size).c_str());
                    ImGui::Separator();

                    // Generic "Open" for any registered viewer type
                    if (wad.fileSource && ctx.viewerRegistry.CanHandle(entry.schemaType)) {
                        if (ImGui::MenuItem("\xEE\xA8\xAA  Open")) {
                            auto viewer = ctx.viewerRegistry.Open(entry, wad);
                            if (viewer) ctx.documentWindow.AddTab(viewer);
                        }
                    }

                    // Type-specific extras: "View All Textures" for MDL with TXR children
                    if (entry.schemaType == "GOW2_MDL" && has_children && wad.fileSource) {
                        int txrCount = 0;
                        for (const auto& c : entry.children) {
                            if (c.schemaType == "GOW2_TXR") txrCount++;
                        }
                        if (txrCount > 0) {
                            char menuLabel[64];
                            snprintf(menuLabel, sizeof(menuLabel), "\xEE\xA9\x88  View All Textures (%d)", txrCount);
                            if (ImGui::MenuItem(menuLabel)) {
                                for (const auto& c : entry.children) {
                                    if (c.schemaType == "GOW2_TXR") {
                                        auto viewer = ctx.viewerRegistry.Open(c, wad);
                                        if (viewer) ctx.documentWindow.AddTab(viewer);
                                    }
                                }
                            }
                        }
                    }

                    if (ImGui::MenuItem("\xEE\xA8\x83  Copy Name")) {
                        ImGui::SetClipboardText(entry.name.c_str());
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