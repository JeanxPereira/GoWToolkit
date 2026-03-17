#include "ui/InfoTab.h"
#include "UIHelpers.h"
#include "imgui.h"
#include <string>
#include <sstream>
#include <iomanip>

// helper: read-only property row
static void PropRow(const char* key, const std::string& value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextDisabled("%s", key);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(value.c_str());
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

void InfoTab::Draw(AssetDatabase& db, ParsedEntry* e) {
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
        PropRow("Type",     TypeName(e->schemaType));
        PropRow("WAD",      e->wadName);
        PropRow("Size",     FormatBytes(e->size));
        PropRow("Offset",   FormatHex32(e->offset));
        if (e->hash != 0)
            PropRow("Hash", HashHex(e->hash));

        ImGui::EndTable();
    }

    // If there's a loaded instance with a StructDef, show its fields read-only
    if (!e->instance) return;
    auto def = e->instance->GetDef();
    if (!def) return;

    ImGui::Spacing();
    ImGui::SeparatorText("Properties");
    ImGui::Spacing();

    if (ImGui::BeginTable("##struct_fields", 2,
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableSetupColumn("Field",  ImGuiTableColumnFlags_WidthFixed, 160);
        ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch);

        size_t currentOffset = 0;
        for (const auto& field : def->GetFields()) {
            // Skip padding / unknown fields
            if (field.name.find("Pad") == 0 || field.name.find("pad") == 0 ||
                field.name.find("Unk") == 0 || field.name.find("unk") == 0 ||
                field.name.find("Padding") == 0) {
                currentOffset += GOW::GetFieldTypeSize(field.type) * field.count;
                continue;
            }

            switch(field.type) {
                case GOW::FieldType::Int32: {
                    int32_t val = e->instance->GetValue<int32_t>(currentOffset);
                    PropRow(field.name.c_str(), std::to_string(val));
                    currentOffset += 4 * field.count;
                    break;
                }
                case GOW::FieldType::UInt32: {
                    uint32_t val = e->instance->GetValue<uint32_t>(currentOffset);
                    PropRow(field.name.c_str(), std::to_string(val) + "  (" + FormatHex32(val) + ")");
                    currentOffset += 4 * field.count;
                    break;
                }
                case GOW::FieldType::Float: {
                    float val = e->instance->GetValue<float>(currentOffset);
                    PropRow(field.name.c_str(), FormatFloat(val));
                    currentOffset += 4 * field.count;
                    break;
                }
                case GOW::FieldType::Vector3: {
                    float x = e->instance->GetValue<float>(currentOffset);
                    float y = e->instance->GetValue<float>(currentOffset + 4);
                    float z = e->instance->GetValue<float>(currentOffset + 8);
                    std::string val = FormatFloat(x) + ", " + FormatFloat(y) + ", " + FormatFloat(z);
                    PropRow(field.name.c_str(), val);
                    currentOffset += 12 * field.count;
                    break;
                }
                case GOW::FieldType::UInt64: {
                    uint64_t val = e->instance->GetValue<uint64_t>(currentOffset);
                    std::stringstream ss;
                    ss << "0x" << std::hex << std::setfill('0') << std::setw(16) << val;
                    PropRow(field.name.c_str(), ss.str());
                    currentOffset += 8 * field.count;
                    break;
                }
                case GOW::FieldType::HexDump: {
                    // Skip hex dumps entirely in the clean view
                    currentOffset += field.count;
                    break;
                }
                default:
                    currentOffset += GOW::GetFieldTypeSize(field.type) * field.count;
                    break;
            }
        }
        ImGui::EndTable();
    }
}