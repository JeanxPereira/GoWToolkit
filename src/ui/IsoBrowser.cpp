#include "ui/IsoBrowser.h"
#include "ui/AppContext.h"
#include "core/vfs/IsoFileSystem.h"
#include "UIHelpers.h"
#include "imgui.h"
#include <filesystem>

void IsoBrowser::draw(AppContext& ctx) {
    if (!visible) return;

    ImGui::Begin("ISO Browser", &visible);

    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##iso_filter", "Filter...", m_filter, sizeof(m_filter));
    ImGui::Separator();

    auto& db = ctx.db;

    for (size_t i = 0; i < db.isos.size(); i++) {
        auto& iso = db.isos[i];

        std::string filename = std::filesystem::path(iso->GetPath()).filename().string();

        bool open = ImGui::TreeNodeEx(filename.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 16);
        if (ImGui::SmallButton("x")) {
            db.CloseIso(i);
            if (open) ImGui::TreePop();
            continue;
        }

        if (!open) continue;

        for (const auto& [path, entry] : iso->GetEntries()) {
            if (entry.name.empty() || entry.name == "." || entry.name == "..") continue;

            if (m_filter[0] && !MatchesFilter(entry.name, m_filter))
                continue;

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;

            const char* icon = entry.isDirectory ? (const char*)u8"\uea83" : (const char*)u8"\uea7b";

            ImGui::TreeNodeEx((std::string(icon) + " " + path).c_str(), flags);

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Size: %s", FormatBytes(entry.size).c_str());
                ImGui::Text("LBA: %u", entry.lba);
                ImGui::EndTooltip();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !entry.isDirectory) {
                db.LoadWad(iso->GetPath());
                ImGui::SetWindowFocus("PAK Browser");
            }

            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    ImGui::End();
}
