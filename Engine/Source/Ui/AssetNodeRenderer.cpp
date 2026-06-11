#include "Ui/AssetNodeRenderer.h"
#include "imgui.h"
#include <cstdio>

namespace Onyx {

// ── Forward declaration ────────────────────────────────────────────────────
static void RenderAssetNodeRow(const Schema::AssetNode& node);

// ── Render an entire AssetNode tree ──────────────────────────────────────────
void RenderAssetNode(const Schema::AssetNode& node) {
    if (node.Kind() == Schema::NodeKind::Struct || node.Kind() == Schema::NodeKind::Array) {
        for (auto& child : node.children)
            RenderAssetNodeRow(*child);
        return;
    }
    RenderAssetNodeRow(node);
}

// ── Render a single node row ──────────────────────────────────────────────────
static void RenderAssetNodeRow(const Schema::AssetNode& node) {
    ImGui::PushID(node.binaryOffset);

    switch (node.Kind()) {

        // ── Struct (treenode with children) ─────────────────────────────────
        case Schema::NodeKind::Struct: {
            auto& n = static_cast<const Schema::StructNode&>(node);
            char label[256];
            snprintf(label, sizeof(label), "%s  [%s]", n.name.c_str(), n.typeName.c_str());
            if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto& child : n.children)
                    RenderAssetNodeRow(*child);
                ImGui::TreePop();
            }
            break;
        }

        // ── Array ────────────────────────────────────────────────────────────
        case Schema::NodeKind::Array: {
            auto& n = static_cast<const Schema::ArrayNode&>(node);
            char label[256];
            snprintf(label, sizeof(label), "%s  [%zu items]",
                     n.name.c_str(), n.children.size());
            if (ImGui::TreeNode(label)) {
                for (size_t i = 0; i < n.children.size(); i++) {
                    ImGui::PushID((int)i);
                    RenderAssetNodeRow(*n.children[i]);
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            break;
        }

        // ── Bool (checkbox) ──────────────────────────────────────────────────
        case Schema::NodeKind::Bool: {
            auto& n = static_cast<const Schema::BoolNode&>(node);
            bool v = n.value;
            ImGui::BeginDisabled();
            ImGui::Checkbox(n.name.c_str(), &v);
            ImGui::EndDisabled();
            break;
        }

        // ── Vector (drag floats or color square) ─────────────────────────────
        case Schema::NodeKind::Vector: {
            auto& n = static_cast<const Schema::VectorNode&>(node);
            float v[4] = {n.x, n.y, n.z, n.w};

            if (n.display == Schema::DisplayHint::Color) {
                if (n.componentCount >= 4)
                    ImGui::ColorEdit4(n.name.c_str(), v,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
                else
                    ImGui::ColorEdit3(n.name.c_str(), v,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
            } else {
                ImGui::BeginDisabled();
                if (n.componentCount == 2)
                    ImGui::DragFloat2(n.name.c_str(), v, 0, 0, 0, "%.4f");
                else if (n.componentCount == 3)
                    ImGui::DragFloat3(n.name.c_str(), v, 0, 0, 0, "%.4f");
                else
                    ImGui::DragFloat4(n.name.c_str(), v, 0, 0, 0, "%.4f");
                ImGui::EndDisabled();
            }
            break;
        }

        // ── Enum ─────────────────────────────────────────────────────────────
        case Schema::NodeKind::Enum: {
            auto& n = static_cast<const Schema::EnumNode&>(node);
            ImGui::LabelText(n.name.c_str(), "%s", n.DisplayValue().c_str());
            break;
        }

        // ── Number ───────────────────────────────────────────────────────────
        case Schema::NodeKind::Number: {
            auto& n = static_cast<const Schema::NumberNode&>(node);
            if (n.originalType == Schema::DataType::Float) {
                float v = n.AsFloat();
                ImGui::BeginDisabled();
                ImGui::DragFloat(n.name.c_str(), &v, 0, 0, 0, "%.4f");
                ImGui::EndDisabled();
            } else {
                ImGui::LabelText(n.name.c_str(), "%s", n.DisplayValue().c_str());
            }
            break;
        }

        // ── String ───────────────────────────────────────────────────────────
        case Schema::NodeKind::String: {
            auto& n = static_cast<const Schema::StringNode&>(node);
            ImGui::LabelText(n.name.c_str(), "%s", n.value.c_str());
            break;
        }

        // ── Hex Dump (collapsed tree) ────────────────────────────────────────
        case Schema::NodeKind::Hex: {
            auto& n = static_cast<const Schema::HexNode&>(node);
            if (ImGui::TreeNode(n.name.c_str())) {
                // Render hex dump in rows of 16 bytes
                const size_t rowSize = 16;
                for (size_t row = 0; row < n.data.size(); row += rowSize) {
                    char line[128] = {};
                    int pos = snprintf(line, sizeof(line), "%04X:  ", (unsigned int)row);
                    size_t end = std::min(row + rowSize, n.data.size());
                    for (size_t i = row; i < end; i++)
                        pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", n.data[i]);
                    ImGui::TextDisabled("%s", line);
                }
                ImGui::TreePop();
            }
            break;
        }

        // ── Null ─────────────────────────────────────────────────────────────
        case Schema::NodeKind::Null: {
            ImGui::LabelText(node.name.c_str(), "(null)");
            break;
        }
    }

    // Tooltip with binary offset on hover
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::TextDisabled("offset: 0x%04X  |  %s",
                            node.binaryOffset,
                            Schema::DataTypeName(
                                node.Kind() == Schema::NodeKind::Number
                                    ? static_cast<const Schema::NumberNode&>(node).originalType
                                    : node.Kind() == Schema::NodeKind::Vector ? Schema::DataType::Vec3
                                    : node.Kind() == Schema::NodeKind::Bool   ? Schema::DataType::Bool
                                    : Schema::DataType::String));
        ImGui::EndTooltip();
    }

    ImGui::PopID();
}

} // namespace Onyx
