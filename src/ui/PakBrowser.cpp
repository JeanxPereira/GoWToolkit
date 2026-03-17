#include "ui/PakBrowser.h"
#include "ui/AppContext.h"
#include "core/AssetDatabase.h"
#include "ui/ViewerRegistry.h"
#include "ui/viewers/DocumentWindow.h"
#include "UIHelpers.h"
#include "imgui.h"

void PakBrowser::draw(AppContext& ctx) {
    if (!visible) return;

    ImGui::Begin("PAK Browser", &visible);

    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##pak_filter", "Filter...", m_filter, sizeof(m_filter));

    ImGui::Separator();

    auto& db = ctx.db;

    for (size_t pi = 0; pi < db.paks.size(); pi++) {
        auto& pak = db.paks[pi];
        bool open = ImGui::TreeNodeEx(pak.filename.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 16);
        ImGui::PushID((int)pi);
        if (ImGui::SmallButton("x")) {
            db.ClosePak(pi);
            ImGui::PopID();
            if (open) ImGui::TreePop();
            continue;
        }
        ImGui::PopID();

        if (!open) continue;

        for (auto& entry : pak.entries) {
            if (m_filter[0] && !MatchesFilter(entry.name, m_filter))
                continue;

            const char* icon = IconForType(entry.schemaType);
            bool is_selected = (&entry == ctx.selected);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth | (is_selected ? ImGuiTreeNodeFlags_Selected : 0);

            ImGui::PushStyleColor(ImGuiCol_Text, ColorForType(entry.schemaType));
            ImGui::TreeNodeEx((std::string(icon) + " " + entry.name).c_str(), flags);
            ImGui::PopStyleColor();
            ImGui::TreePop();

            if (ImGui::IsItemClicked()) {
                ctx.selected = &entry;
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Pak: %s", entry.wadName.c_str());
                ImGui::Text("Offset: 0x%08X", entry.offset);
                ImGui::Text("Size: %s", FormatBytes(entry.size).c_str());
                ImGui::EndTooltip();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                const auto& type = entry.schemaType;

                if (type == "GOW2_WAD_FILE" || type == "UNKNOWN") {
                    db.LoadWadFromPakEntry(&entry, pak);
                    ImGui::SetWindowFocus("WAD Browser");
                } else {
                    if (ctx.viewerRegistry.CanHandle(type)) {
                        auto fileHandle = db.OpenPakEntryAsFile(&entry, pak);
                        if (fileHandle) {
                            OpenWad tempWad;
                            tempWad.filename = entry.name;
                            tempWad.fullPath = pak.fullPath;
                            tempWad.profile = pak.profile;
                            tempWad.fileSource = fileHandle;

                            ParsedEntry fileEntry = entry;
                            fileEntry.offset = 0;
                            tempWad.entries.push_back(fileEntry);

                            auto viewer = ctx.viewerRegistry.Open(entry, tempWad);
                            if (viewer) {
                                ctx.documentWindow.AddTab(viewer);
                            }
                        }
                    } else {
                        db.LoadWadFromPakEntry(&entry, pak);
                        ImGui::SetWindowFocus("WAD Browser");
                    }
                }
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();
}
