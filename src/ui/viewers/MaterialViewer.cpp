#include "MaterialViewer.h"
#include <imgui.h>

namespace GOW {

MaterialViewer::MaterialViewer(const std::string& name, std::unique_ptr<GOW2MaterialParser::MaterialData> matData)
    : m_name(name), m_matData(std::move(matData)) {}

std::string MaterialViewer::GetName() const {
    return "\xEE\xA9\x91 " + m_name;
}

void MaterialViewer::Draw() {
    if (!m_matData) {
        ImGui::Text("Invalid Material Data");
        return;
    }

    ImGui::Text("Material Layers: %zu", m_matData->layers.size());
    ImGui::Separator();

    if (ImGui::BeginTable("MatsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Has Texture");
        ImGui::TableSetupColumn("Texture Name");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < m_matData->layers.size(); ++i) {
            const auto& layer = m_matData->layers[i];
            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();
            ImGui::Text("%zu", i);
            
            ImGui::TableNextColumn();
            ImGui::Text(layer.hasTexture ? "Yes" : "No");

            ImGui::TableNextColumn();
            if (layer.hasTexture && !layer.textureName.empty()) {
                ImGui::Text("%s", layer.textureName.c_str());
            } else {
                ImGui::TextDisabled("None");
            }
        }

        ImGui::EndTable();
    }
}

} // namespace GOW
