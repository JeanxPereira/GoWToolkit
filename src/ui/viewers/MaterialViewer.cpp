#include "MaterialViewer.h"
#include "fonts/SFSymbols.h"
#include <glad/glad.h>
#include <imgui.h>

namespace GOW {

MaterialViewer::MaterialViewer(
    const std::string &name,
    std::unique_ptr<GOW2MaterialParser::MaterialData> matData,
    TextureLookupFn texLookup,
    std::vector<std::unique_ptr<TextureData>> textures)
    : m_name(name), m_matData(std::move(matData)), m_texLookup(texLookup),
      m_textures(std::move(textures)) {
  UploadTextures();
}

MaterialViewer::~MaterialViewer() {
  if (!m_glTextures.empty()) {
    glDeleteTextures((GLsizei)m_glTextures.size(), m_glTextures.data());
  }
}

std::string MaterialViewer::GetName() const {
  return ICON_SF_PAINTPALETTE_FILL "  " + m_name;
}

void MaterialViewer::UploadTextures() {
  m_glTextures.resize(m_textures.size(), 0);
  for (size_t i = 0; i < m_textures.size(); ++i) {
    const auto &tex = m_textures[i];
    if (tex && tex->IsValid()) {
      GLuint glTex;
      glGenTextures(1, &glTex);
      glBindTexture(GL_TEXTURE_2D, glTex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->width, tex->height, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, tex->pixels.data());
      m_glTextures[i] = glTex;
    }
  }
  glBindTexture(GL_TEXTURE_2D, 0);
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
    GLuint previewTexId = 0;
    std::string previewInfo = "No texture";

    if (m_selectedLayer >= 0 &&
        m_selectedLayer < (int)m_matData->layers.size()) {
      const auto &layer = m_matData->layers[m_selectedLayer];

      // Try direct GL texture from m_glTextures first
      if (m_selectedLayer < (int)m_glTextures.size() &&
          m_glTextures[m_selectedLayer] != 0) {
        previewTexId = m_glTextures[m_selectedLayer];
        if (m_selectedLayer < (int)m_textures.size() &&
            m_textures[m_selectedLayer]) {
          previewInfo =
              layer.textureName + " (" +
              std::to_string(m_textures[m_selectedLayer]->width) + "x" +
              std::to_string(m_textures[m_selectedLayer]->height) + ")";
        }
      }
      // Fall back to texLookup
      else if (layer.hasTexture && m_texLookup) {
        previewTexId = m_texLookup(layer.textureName);
        if (previewTexId != 0) {
          previewInfo = layer.textureName;
        }
      }
    }

    if (previewTexId != 0) {
      float avail = ImGui::GetContentRegionAvail().x;
      float imgSize = avail - 8.0f;
      if (imgSize < 64.0f)
        imgSize = 64.0f;

      // Checkerboard background hint
      ImVec2 pos = ImGui::GetCursorScreenPos();
      ImDrawList *dl = ImGui::GetWindowDrawList();
      dl->AddRectFilled(pos, ImVec2(pos.x + imgSize, pos.y + imgSize),
                        IM_COL32(40, 40, 40, 255));

      ImGui::Image((void *)(intptr_t)previewTexId, ImVec2(imgSize, imgSize));
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
        if (ImGui::Selectable(layerLabel, isSelected,
                              ImGuiSelectableFlags_SpanAllColumns)) {
          m_selectedLayer = (int)i;
        }

        // Thumbnail
        ImGui::TableNextColumn();
        GLuint thumbTexId = 0;
        if (i < m_glTextures.size() && m_glTextures[i] != 0) {
          thumbTexId = m_glTextures[i];
        } else if (layer.hasTexture && m_texLookup) {
          thumbTexId = m_texLookup(layer.textureName);
        }
        if (thumbTexId != 0) {
          ImGui::Image((void *)(intptr_t)thumbTexId, ImVec2(60, 60));
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

} // namespace GOW