#include "Ui/InfoTab.h"
#include "UIHelpers.h"
#include "imgui.h"
#include "Ui/AssetNodeRenderer.h"
#include <string>
#include <sstream>
#include <iomanip>

// helper: read-only property row
static void PropRow(const char* key, const std::string& value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextDisabled("%s", key);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextWrapped("%s", value.c_str());
    // Copy on click
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
        ImGui::SetClipboardText(value.c_str());
    }
}

static std::string FormatHex32(uint32_t v) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << v;
    return ss.str();
}

static std::string FormatFloat(float v) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4) << v;
    return ss.str();
}

void InfoTab::Draw(AssetDatabase& db, AssetEntry* e) {
    if (!e) return;

    // Basic metadata table (always shown, read-only)
    if (ImGui::BeginTable("##props", 2,
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableSetupColumn("Key",   ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        PropRow("Name",     e->name);
        PropRow("Type",     TypeName(e->typeId));
        PropRow("WAD",      e->wadName);
        PropRow("Size",     FormatBytes(e->size));
        PropRow("Offset",   FormatHex32(e->offset));
        if (e->hash != 0)
            PropRow("Hash", HashHex(e->hash));

        ImGui::EndTable();
    }

    // If there's a loaded AssetNode tree, render it automatically
    if (!e->assetNode) return;

    ImGui::Spacing();
    ImGui::SeparatorText("Properties");
    ImGui::Spacing();

    // Render the tree using the auto-renderer
    Onyx::RenderAssetNode(*e->assetNode);
}
