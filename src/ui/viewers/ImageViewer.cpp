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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_texture->width, m_texture->height,
               0, GL_RGBA, GL_UNSIGNED_BYTE, m_texture->pixels.data());
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
