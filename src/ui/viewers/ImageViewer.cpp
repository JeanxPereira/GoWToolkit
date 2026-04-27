#include "ImageViewer.h"
#include "fonts/SFSymbols.h"
#include <algorithm>
#include <glad/glad.h>
#include <imgui.h>


namespace GOW {

ImageViewer::ImageViewer(const std::string &name,
                         std::unique_ptr<TextureData> texture)
    : m_name(name), m_texture(std::move(texture)) {
  if (m_texture && m_texture->IsValid()) {
    UploadToGPU();
  }
}

ImageViewer::~ImageViewer() {
  if (m_glTexture) {
    glDeleteTextures(1, &m_glTexture);
  }
}

std::string ImageViewer::GetName() const { return m_name; }

void ImageViewer::UploadToGPU() {
  if (!m_texture || !m_texture->IsValid())
    return;

  glGenTextures(1, &m_glTexture);
  glBindTexture(GL_TEXTURE_2D, m_glTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  if (m_texture->isCompressed) {
      // Calculate correct mip0 size based on format
      // BC1/BC4: 8 bytes per 4x4 block, BC2/BC3/BC5/BC6/BC7: 16 bytes per 4x4 block
      uint32_t blockW = (m_texture->width + 3) / 4;
      uint32_t blockH = (m_texture->height + 3) / 4;
      uint32_t bytesPerBlock = 16; // default for BC7, BC3, BC5, BC6
      uint32_t fmt = m_texture->glInternalFormat;
      if (fmt == 0x83F0 || fmt == 0x83F1 || fmt == 0x8C4C || fmt == 0x8C4D ||  // BC1
          fmt == 0x8DBB || fmt == 0x8DBC) {                                       // BC4
          bytesPerBlock = 8;
      }
      uint32_t mip0Size = blockW * blockH * bytesPerBlock;
      
      // Only upload mip0 (decSize from block may include all mips)
      uint32_t uploadSize = std::min(mip0Size, static_cast<uint32_t>(m_texture->pixels.size()));
      
      glCompressedTexImage2D(GL_TEXTURE_2D, 0, m_texture->glInternalFormat,
                             m_texture->width, m_texture->height,
                             0, uploadSize, m_texture->pixels.data());
      
      GLenum err = glGetError();
      if (err != GL_NO_ERROR) {
          printf("[ImageViewer] glCompressedTexImage2D error: 0x%X (fmt=0x%X %ux%u %u bytes)\n",
                 err, m_texture->glInternalFormat, m_texture->width, m_texture->height, uploadSize);
      }
      
      // Set swizzle masks for single/dual channel formats
      // BC4 (RGTC1): single Red channel → display as grayscale (R,R,R,1)
      if (fmt == 0x8DBB || fmt == 0x8DBC) {
          GLint swizzle[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
          glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
      }
      // BC5 (RGTC2): RG channels → display as normal map (R,G,1,1)
      else if (fmt == 0x8DBD || fmt == 0x8DBE) {
          GLint swizzle[] = { GL_RED, GL_GREEN, GL_ONE, GL_ONE };
          glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
      }
  } else {
      glTexImage2D(GL_TEXTURE_2D, 0, m_texture->glInternalFormat, m_texture->width, m_texture->height,
                   0, GL_RGBA, GL_UNSIGNED_BYTE, m_texture->pixels.data());
  }
  glBindTexture(GL_TEXTURE_2D, 0);
}

void ImageViewer::Draw() {
  if (!m_texture || !m_glTexture) {
    ImGui::TextDisabled("No texture data");
    return;
  }

  // Toolbar
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 0.85f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.25f, 0.25f, 0.3f, 0.9f));

  ImGui::TextDisabled("%ux%u", m_texture->width, m_texture->height);
  ImGui::SameLine();

  if (ImGui::SmallButton(ICON_SF_PLUS_MAGNIFYINGGLASS))
    m_zoom = std::min(m_zoom * 1.5f, 16.0f);
  ImGui::SameLine();
  if (ImGui::SmallButton(ICON_SF_MINUS_MAGNIFYINGGLASS))
    m_zoom = std::max(m_zoom / 1.5f, 0.125f);
  ImGui::SameLine();
  if (ImGui::SmallButton("1:1"))
    m_zoom = 1.0f;
  ImGui::SameLine();
  if (ImGui::SmallButton("Fit")) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float scaleX = avail.x / m_texture->width;
    float scaleY = avail.y / m_texture->height;
    m_zoom = std::min(scaleX, scaleY);
  }
  ImGui::SameLine();
  ImGui::Checkbox("Alpha", &m_showAlpha);

  ImGui::PopStyleColor(2);
  ImGui::Separator();

  // Image display
  ImVec2 avail = ImGui::GetContentRegionAvail();
  float imgW = m_texture->width * m_zoom;
  float imgH = m_texture->height * m_zoom;

  // Center the image if it's smaller than the viewport
  ImVec2 cursor = ImGui::GetCursorScreenPos();
  float offsetX = (avail.x > imgW) ? (avail.x - imgW) * 0.5f : 0.0f;
  float offsetY = (avail.y > imgH) ? (avail.y - imgH) * 0.5f : 0.0f;

  // Checkerboard background for alpha
  ImGui::BeginChild("##texview", avail, false,
                    ImGuiWindowFlags_HorizontalScrollbar);

  if (offsetX > 0 || offsetY > 0) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
  }

  ImGui::Image((void *)(intptr_t)m_glTexture, ImVec2(imgW, imgH), ImVec2(0, 0),
               ImVec2(1, 1));

  // Scroll zoom
  if (ImGui::IsItemHovered()) {
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel > 0)
      m_zoom = std::min(m_zoom * 1.2f, 16.0f);
    if (wheel < 0)
      m_zoom = std::max(m_zoom / 1.2f, 0.125f);
  }

  ImGui::EndChild();
}

} // namespace GOW
