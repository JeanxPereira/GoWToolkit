#include "Ui/IsoBrowser.h"
#include "Core/ToolkitApi.h"
#include "Fonts/SFSymbols.h"
#include "Core/Vfs/IsoFileSystem.h"
#include "UIHelpers.h"
#include "Ui/Widgets.h"
#include "imgui.h"
#include <filesystem>

void IsoBrowser::Draw() {
    if (!visible) return;

    ImGui::Begin("ISO Browser", &visible);

    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##iso_filter", "Filter...", m_filter, sizeof(m_filter));
    ImGui::Separator();

    auto& db = Onyx::Api::Database();

    for (size_t i = 0; i < db.isos.size(); i++) {
        auto& iso = db.isos[i];

        std::string filename = std::filesystem::path(iso->GetPath()).filename().string();

        ImGui::PushID((int)i);

        bool open = ImGui::TreeNodeEx(filename.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight());
        Onyx::UI::Widgets::IconButtonOpts closeOpts;
        closeOpts.tooltip = "Close ISO";
        if (Onyx::UI::Widgets::IconButton("iso_close", ICON_SF_XMARK, closeOpts)) {
            db.CloseIso(i);
            if (open) ImGui::TreePop();
            ImGui::PopID();
            continue;
        }

        if (!open) {
            ImGui::PopID();
            continue;
        }

        const ImVec4 neutralColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);

        int rowIdx = 0;
        for (const auto& [path, entry] : iso->GetEntries()) {
            if (entry.name.empty() || entry.name == "." || entry.name == "..") continue;

            if (m_filter[0] && !MatchesFilter(entry.name, m_filter))
                continue;

            const char* icon = entry.isDirectory ? ICON_SF_FOLDER_FILL : ICON_SF_DOCUMENT;

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                ImGuiTreeNodeFlags_SpanFullWidth;

            char rowId[24];
            snprintf(rowId, sizeof(rowId), "%d", rowIdx++);
            Onyx::UI::Widgets::ColoredTreeNode(rowId, path.c_str(), icon, neutralColor, flags, false);

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Size: %s", FormatBytes(entry.size).c_str());
                ImGui::Text("Offset: 0x%llX", (unsigned long long)entry.offset);
                ImGui::EndTooltip();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !entry.isDirectory) {
                db.LoadWad(iso->GetPath());
                ImGui::SetWindowFocus("PAK Browser");
            }
        }
        ImGui::TreePop();
        ImGui::PopID();
    }

    ImGui::End();
}
