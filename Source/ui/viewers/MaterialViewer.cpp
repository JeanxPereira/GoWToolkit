#include "MaterialViewer.h"
#include <Onyx/Fonts/SFSymbols.h>
#include <Onyx/App/Widgets.h>
#include <Onyx/App/TexturePool.h>
#include <Onyx/Rendering/VkContext.h>
#include <Onyx/Services/Logger.h>
#include <imgui.h>

namespace Onyx {

MaterialViewer::MaterialViewer(
    const std::string &name,
    std::unique_ptr<GOW2MaterialParser::MaterialData> matData,
    std::vector<std::unique_ptr<Parsers::TextureData>> textures)
    : m_name(name), m_matData(std::move(matData)),
      m_textures(std::move(textures)) {
  UploadTextures();
}

// Destroying the pool releases every texture it created -- no explicit
// per-texture free, unlike the glDeleteTextures this replaces.
MaterialViewer::~MaterialViewer() = default;

std::string MaterialViewer::GetName() const {
  return ICON_SF_PAINTPALETTE_FILL "  " + m_name;
}

void MaterialViewer::UploadTextures() {
  m_texIds.assign(m_textures.size(), ImTextureID_Invalid);

  Onyx::Rendering::VkContext *ctx = Onyx::Rendering::GetGlobalContext();
  if (!ctx) {
    ONYX_LOGF_ERR("[MaterialViewer] '%s': no live VkContext -- textures not uploaded",
                  m_name.c_str());
    return;
  }
  m_texPool = std::make_unique<Onyx::App::TexturePool>(*ctx);

  for (size_t i = 0; i < m_textures.size(); ++i) {
    const auto &tex = m_textures[i];
    if (!tex || !tex->IsValid()) continue;

    std::string err;
    ImTextureID id = m_texPool->Create(tex->width, tex->height,
                                       tex->pixels.data(), err);
    if (id == ImTextureID_Invalid) {
      ONYX_LOGF_ERR("[MaterialViewer] '%s': layer %zu upload failed: %s",
                    m_name.c_str(), i, err.c_str());
      continue;
    }
    m_texIds[i] = id;
  }
}

void MaterialViewer::Draw() {
  if (!m_matData) {
    ImGui::Text("Invalid Material Data");
    return;
  }

  // ── Layout: Preview panel (left) + Table (right) ──────────────
  float totalWidth = ImGui::GetContentRegionAvail().x;
  float previewWidth = totalWidth * 0.4f;
  if (previewWidth < 180.0f)
    previewWidth = 180.0f;
  if (previewWidth > 400.0f)
    previewWidth = 400.0f;

  // ── Left: Texture Preview ─────────────────────────────────────
  ImGui::BeginChild("##texPreview", ImVec2(previewWidth, 0), true);
  {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Texture Preview");
    ImGui::Separator();

    // Find texture for selected layer
    ImTextureID previewTexId = ImTextureID_Invalid;
    std::string previewInfo = "No texture";

    if (m_selectedLayer >= 0 &&
        m_selectedLayer < (int)m_matData->layers.size()) {
      const auto &layer = m_matData->layers[m_selectedLayer];

      if (m_selectedLayer < (int)m_texIds.size() &&
          m_texIds[m_selectedLayer] != ImTextureID_Invalid) {
        previewTexId = m_texIds[m_selectedLayer];
        if (m_selectedLayer < (int)m_textures.size() &&
            m_textures[m_selectedLayer]) {
          previewInfo =
              layer.textureName + " (" +
              std::to_string(m_textures[m_selectedLayer]->width) + "x" +
              std::to_string(m_textures[m_selectedLayer]->height) + ")";
        }
      } else if (layer.hasTexture) {
        previewInfo = layer.textureName + " (not decoded)";
      }
    }

    if (previewTexId != ImTextureID_Invalid) {
      float avail = ImGui::GetContentRegionAvail().x;
      float imgSize = avail - 8.0f;
      if (imgSize < 64.0f)
        imgSize = 64.0f;

      // Checkerboard background hint
      ImVec2 pos = ImGui::GetCursorScreenPos();
      ImDrawList *dl = ImGui::GetWindowDrawList();
      dl->AddRectFilled(pos, ImVec2(pos.x + imgSize, pos.y + imgSize),
                        IM_COL32(40, 40, 40, 255));

      ImGui::Image(previewTexId, ImVec2(imgSize, imgSize));
      ImGui::TextWrapped("%s", previewInfo.c_str());
    } else {
      ImGui::TextDisabled("Select a layer with a\ntexture to preview");
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Layer %d / %d", m_selectedLayer + 1,
                        (int)m_matData->layers.size());
  }
  ImGui::EndChild();

  ImGui::SameLine();

  // ── Right: Material Properties + Table ────────────────────────
  ImGui::BeginChild("##matProps", ImVec2(0, 0), false);
  {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Material Summary");

    // Base Color
    ImGui::Text("Base Color: ");
    ImGui::SameLine();
    ImGui::ColorButton("##base", *(ImVec4 *)m_matData->baseColor,
                       ImGuiColorEditFlags_NoPicker);
    ImGui::SameLine();
    ImGui::Text("(%.2f, %.2f, %.2f)", m_matData->baseColor[0],
                m_matData->baseColor[1], m_matData->baseColor[2]);

    ImGui::Separator();

    if (ImGui::BeginTable("MatsTable", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable)) {
      ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_WidthFixed, 40.0f);
      ImGui::TableSetupColumn("Visual", ImGuiTableColumnFlags_WidthFixed,
                              65.0f);
      ImGui::TableSetupColumn("Mode / Numerical Color",
                              ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Texture Details",
                              ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableHeadersRow();

      for (size_t i = 0; i < m_matData->layers.size(); ++i) {
        const auto &layer = m_matData->layers[i];
        ImGui::TableNextRow(ImGuiTableRowFlags_None, 65.0f);

        // Highlight selected layer
        bool isSelected = ((int)i == m_selectedLayer);
        if (isSelected) {
          ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                                 IM_COL32(60, 80, 120, 180));
        }

        ImGui::TableNextColumn();
        // Make the layer number clickable
        char layerLabel[32];
        snprintf(layerLabel, sizeof(layerLabel), "%zu##layer", i);
        if (Onyx::App::Widgets::Selectable(layerLabel, isSelected,
                              ImGuiSelectableFlags_SpanAllColumns)) {
          m_selectedLayer = (int)i;
        }

        // Thumbnail
        ImGui::TableNextColumn();
        ImTextureID thumbTexId =
            (i < m_texIds.size()) ? m_texIds[i] : ImTextureID_Invalid;
        if (thumbTexId != ImTextureID_Invalid) {
          ImGui::Image(thumbTexId, ImVec2(60, 60));
        } else if (layer.hasTexture) {
          ImGui::TextDisabled("N/A");
        }

        ImGui::TableNextColumn();
        const char *blendModes[] = {"Usual/Alpha", "Additive", "Subtract",
                                    "Strange"};
        ImGui::TextDisabled("Mode: %s", blendModes[layer.renderingMethod]);

        // Color Button
        ImGui::ColorButton("##lclr", *(ImVec4 *)layer.blendColor,
                           ImGuiColorEditFlags_NoPicker);
        ImGui::SameLine();
        ImGui::Text("RGBA: %.2f, %.2f, %.2f, %.2f", layer.blendColor[0],
                    layer.blendColor[1], layer.blendColor[2],
                    layer.blendColor[3]);

        ImGui::TableNextColumn();
        if (layer.hasTexture) {
          ImGui::Text("%s", layer.textureName.c_str());
          if (layer.uvAnimEnabled)
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "[UV ANIM]");
          if (layer.colorAnimEnabled)
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "[CLR ANIM]");
        } else {
          ImGui::TextDisabled("No Texture");
        }
      }
      ImGui::EndTable();
    }
  }
  ImGui::EndChild();
}

} // namespace Onyx
