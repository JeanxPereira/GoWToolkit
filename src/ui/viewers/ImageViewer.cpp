#include "ImageViewer.h"
#include "fonts/SFSymbols.h"
#include "core/ThemeManager.h"
#include "ui/Widgets.h"
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

void ImageViewer::ZoomToAnchored(float newZoom, ImVec2 anchorScreen) {
  newZoom = std::clamp(newZoom, 0.125f, 16.0f);
  if (newZoom == m_zoomTarget) return;
  // Image-local coords of the anchor at the current (target) pan.
  const ImVec2 local(anchorScreen.x - m_panTarget.x,
                     anchorScreen.y - m_panTarget.y);
  const float  scale = newZoom / m_zoomTarget;
  m_panTarget.x = anchorScreen.x - local.x * scale;
  m_panTarget.y = anchorScreen.y - local.y * scale;
  m_zoomTarget  = newZoom;
}

void ImageViewer::Draw() {
  if (!m_texture || !m_glTexture) {
    ImGui::TextDisabled("No texture data");
    return;
  }

  const float texW = static_cast<float>(m_texture->width);
  const float texH = static_cast<float>(m_texture->height);

  // ── Toolbar ──────────────────────────────────────────────────────────
  ImGui::PushStyleColor(ImGuiCol_Button, GOW::Theme::ToolbarButton());
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GOW::Theme::ToolbarButtonHover());

  ImGui::TextDisabled("%ux%u", m_texture->width, m_texture->height);
  ImGui::SameLine();

  // Defer button actions until we know `avail` (canvas size) — the viewport
  // center anchor needs it. Bool flags carry the intent across.
  const bool zoomInClicked  = GOW::UI::Widgets::SmallButton(ICON_SF_PLUS_MAGNIFYINGGLASS);
  ImGui::SameLine();
  const bool zoomOutClicked = GOW::UI::Widgets::SmallButton(ICON_SF_MINUS_MAGNIFYINGGLASS);
  ImGui::SameLine();
  const bool oneToOneClicked = GOW::UI::Widgets::SmallButton("1:1");
  ImGui::SameLine();
  const bool fitClicked = GOW::UI::Widgets::SmallButton("Fit");
  ImGui::SameLine();

  // Alpha toggle — show alpha channel as grayscale
  bool prevAlpha = m_showAlpha;
  ImGui::Checkbox("Alpha", &m_showAlpha);
  const bool alphaChanged = (m_showAlpha != prevAlpha);

  ImGui::PopStyleColor(2);
  ImGui::Separator();

  // ── Canvas area (everything below toolbar) ───────────────────────────
  const ImVec2 avail = ImGui::GetContentRegionAvail();
  if (avail.x <= 1.0f || avail.y <= 1.0f) return;

  const ImVec2 origin = ImGui::GetCursorScreenPos();

  // Helpers shared by buttons / first-frame init.
  auto fitZoomFor = [&]() {
    return std::min(avail.x / texW, avail.y / texH);
  };
  auto centerForZoom = [&](float z) {
    return ImVec2((avail.x - texW * z) * 0.5f,
                  (avail.y - texH * z) * 0.5f);
  };
  // Anchor in canvas-local coords (matches m_panTarget convention).
  const ImVec2 viewportCenter(avail.x * 0.5f, avail.y * 0.5f);

  // ── First-frame init: open in Fit mode (no lerp; nothing to ease from) ──
  if (!m_initialFitDone) {
    m_zoomTarget = fitZoomFor();
    m_panTarget  = centerForZoom(m_zoomTarget);
    m_zoom = m_zoomTarget;
    m_pan  = m_panTarget;
    m_initialFitDone = true;
  }

  // ── Toolbar actions (lerp toward target — same path as wheel) ────────
  if (zoomInClicked)   ZoomToAnchored(m_zoomTarget * 1.5f, viewportCenter);
  if (zoomOutClicked)  ZoomToAnchored(m_zoomTarget / 1.5f, viewportCenter);
  if (oneToOneClicked) ZoomToAnchored(1.0f,                 viewportCenter);
  if (fitClicked) {
    m_zoomTarget = fitZoomFor();
    m_panTarget  = centerForZoom(m_zoomTarget);
  }

  ImGui::InvisibleButton("##texcanvas", avail,
                         ImGuiButtonFlags_MouseButtonLeft |
                         ImGuiButtonFlags_MouseButtonMiddle);
  const bool hovered = ImGui::IsItemHovered();
  const bool active  = ImGui::IsItemActive();
  ImGuiIO& io = ImGui::GetIO();

  // Cursor-anchored wheel zoom — same unified helper.
  if (hovered && io.MouseWheel != 0.0f) {
    const float factor = std::pow(1.15f, io.MouseWheel);
    ZoomToAnchored(m_zoomTarget * factor,
                   ImVec2(io.MousePos.x - origin.x,
                          io.MousePos.y - origin.y));
  }

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

  // ── Apply alpha swizzle on the GPU texture when toggle changes ───────
  if (alphaChanged) {
    glBindTexture(GL_TEXTURE_2D, m_glTexture);
    if (m_showAlpha) {
      // Show alpha as grayscale: (A, A, A, 1)
      GLint swizzle[] = { GL_ALPHA, GL_ALPHA, GL_ALPHA, GL_ONE };
      glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    } else {
      // Restore normal RGBA display
      // Re-apply format-specific swizzle if needed
      uint32_t fmt = m_texture->glInternalFormat;
      if (m_texture->isCompressed && (fmt == 0x8DBB || fmt == 0x8DBC)) {
        // BC4: R,R,R,1
        GLint swizzle[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
      } else if (m_texture->isCompressed && (fmt == 0x8DBD || fmt == 0x8DBE)) {
        // BC5: R,G,1,1
        GLint swizzle[] = { GL_RED, GL_GREEN, GL_ONE, GL_ONE };
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
      } else {
        // Normal: R,G,B,A
        GLint swizzle[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
      }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  // ── Draw the image ───────────────────────────────────────────────────
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const ImVec2 p0(origin.x + m_pan.x, origin.y + m_pan.y);
  const ImVec2 p1(p0.x + imgW, p0.y + imgH);
  dl->PushClipRect(origin, ImVec2(origin.x + avail.x, origin.y + avail.y), true);

  // Draw checkerboard background when showing alpha (so transparency is visible)
  if (m_showAlpha) {
    // No checkerboard needed for alpha-as-grayscale mode; just dark bg
    dl->AddRectFilled(p0, p1, IM_COL32(30, 30, 30, 255));
  }

  dl->AddImage((ImTextureID)(intptr_t)m_glTexture, p0, p1);
  dl->PopClipRect();
}

} // namespace GOW

