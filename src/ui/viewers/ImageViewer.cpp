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
    m_zoomTarget = std::min(m_zoomTarget * 1.5f, 16.0f);
  ImGui::SameLine();
  if (ImGui::SmallButton(ICON_SF_MINUS_MAGNIFYINGGLASS))
    m_zoomTarget = std::max(m_zoomTarget / 1.5f, 0.125f);
  ImGui::SameLine();
  if (ImGui::SmallButton("1:1"))
    m_zoomTarget = 1.0f;
  ImGui::SameLine();
  if (ImGui::SmallButton("Fit")) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float scaleX = avail.x / m_texture->width;
    float scaleY = avail.y / m_texture->height;
    m_zoomTarget = std::min(scaleX, scaleY);
  }
  ImGui::SameLine();
  ImGui::Checkbox("Alpha", &m_showAlpha);

  ImGui::PopStyleColor(2);
  ImGui::Separator();

  ImVec2 avail = ImGui::GetContentRegionAvail();
  if (avail.x <= 1.0f || avail.y <= 1.0f) return;

  const ImVec2 origin = ImGui::GetCursorScreenPos();
  ImGui::InvisibleButton("##texcanvas", avail,
                         ImGuiButtonFlags_MouseButtonLeft |
                         ImGuiButtonFlags_MouseButtonMiddle);
  const bool hovered = ImGui::IsItemHovered();
  const bool active  = ImGui::IsItemActive();
  ImGuiIO& io = ImGui::GetIO();

  const float texW = static_cast<float>(m_texture->width);
  const float texH = static_cast<float>(m_texture->height);

  // First-frame init: center image at current zoom.
  if (!m_panInitialized) {
    m_panTarget.x = (avail.x - texW * m_zoomTarget) * 0.5f;
    m_panTarget.y = (avail.y - texH * m_zoomTarget) * 0.5f;
    m_pan  = m_panTarget;
    m_zoom = m_zoomTarget;
    m_panInitialized = true;
  }

  // Cursor-anchored wheel zoom: adjust pan TARGET so the image point currently
  // under the cursor stays under the cursor once zoom finishes lerping.
  if (hovered && io.MouseWheel != 0.0f) {
    const float factor  = std::pow(1.15f, io.MouseWheel);
    const float newZoom = std::clamp(m_zoomTarget * factor, 0.125f, 16.0f);
    if (newZoom != m_zoomTarget) {
      const ImVec2 mouseLocal(io.MousePos.x - origin.x - m_panTarget.x,
                              io.MousePos.y - origin.y - m_panTarget.y);
      const float scale = newZoom / m_zoomTarget;
      m_panTarget.x -= mouseLocal.x * (scale - 1.0f);
      m_panTarget.y -= mouseLocal.y * (scale - 1.0f);
      m_zoomTarget = newZoom;
    }
  }

  // Toolbar zoom buttons recenter the image around viewport center.
  // Drag adds mouse delta to pan target.
  if (active &&
      (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) ||
       ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))) {
    m_panTarget.x += io.MouseDelta.x;
    m_panTarget.y += io.MouseDelta.y;
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
  } else if (hovered) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  }

  // Bounds with margin computed against TARGET zoom so target stays reachable.
  const float kMargin = 80.0f;
  auto clampPan = [&](ImVec2 v, float zoom) {
    const float w = texW * zoom, h = texH * zoom;
    if (w <= avail.x) {
      const float c = (avail.x - w) * 0.5f;
      v.x = std::clamp(v.x, c - kMargin, c + kMargin);
    } else {
      v.x = std::clamp(v.x, avail.x - w - kMargin, kMargin);
    }
    if (h <= avail.y) {
      const float c = (avail.y - h) * 0.5f;
      v.y = std::clamp(v.y, c - kMargin, c + kMargin);
    } else {
      v.y = std::clamp(v.y, avail.y - h - kMargin, kMargin);
    }
    return v;
  };
  m_panTarget = clampPan(m_panTarget, m_zoomTarget);

  // Smooth zoom + pan with identical exp-decay coefficient so the anchor stays
  // consistent throughout the lerp. ~150ms settle.
  const float dt = std::clamp(io.DeltaTime, 1.0f / 240.0f, 1.0f / 30.0f);
  const float k  = 1.0f - std::exp(-18.0f * dt);
  m_zoom  += (m_zoomTarget  - m_zoom)  * k;
  m_pan.x += (m_panTarget.x - m_pan.x) * k;
  m_pan.y += (m_panTarget.y - m_pan.y) * k;

  // Snap when within sub-pixel of target to prevent infinite tiny lerping.
  if (std::abs(m_zoomTarget - m_zoom) < 0.0005f)    m_zoom  = m_zoomTarget;
  if (std::abs(m_panTarget.x - m_pan.x) < 0.25f)    m_pan.x = m_panTarget.x;
  if (std::abs(m_panTarget.y - m_pan.y) < 0.25f)    m_pan.y = m_panTarget.y;

  const float imgW = texW * m_zoom;
  const float imgH = texH * m_zoom;

  ImDrawList* dl = ImGui::GetWindowDrawList();
  const ImVec2 p0(origin.x + m_pan.x, origin.y + m_pan.y);
  const ImVec2 p1(p0.x + imgW, p0.y + imgH);
  dl->PushClipRect(origin, ImVec2(origin.x + avail.x, origin.y + avail.y), true);
  dl->AddImage((ImTextureID)(intptr_t)m_glTexture, p0, p1);
  dl->PopClipRect();
}

} // namespace GOW
