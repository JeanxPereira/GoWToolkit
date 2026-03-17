#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui/TitleBar.h"
#include "ui/NativeWindow.h"
#include "fonts/CodiconIcons.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>
#include <cstring>

// ── TitleBarButton ─────────────────────────────────────────────────────────
// Réplica exacta do ImGuiExt::TitleBarButton do ImHex
bool TitleBarButton(const char* label, ImVec2 size_arg) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext&      g     = *GImGui;
    const ImGuiStyle&  style = g.Style;
    const ImGuiID      id    = window->GetID(label);
    const ImVec2       label_size = ImGui::CalcTextSize(label, nullptr, true);

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(size_arg,
        label_size.x + style.FramePadding.x * 2.0f,
        label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    const ImU32 col = ImGui::GetColorU32(
        (held && hovered) ? ImGuiCol_ButtonActive :
         hovered          ? ImGuiCol_ButtonHovered :
                            ImGuiCol_Button);

    ImGui::RenderNavCursor(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, col, false, style.FrameRounding);
    ImGui::RenderTextClipped(
        bb.Min + style.FramePadding,
        bb.Max - style.FramePadding,
        label, nullptr, &label_size,
        style.ButtonTextAlign, &bb);

    return pressed;
}

namespace TitleBar {

bool  showTitleBarBackDrop = true;
float backdropAlpha        = 0.8f;

static ImFont* s_iconFont = nullptr;

void loadIconFont(const char* ttfPath, float size) {
    ImGuiIO& io = ImGui::GetIO();

    // Garante que existe pelo menos uma font antes do MergeMode
    if (io.Fonts->Fonts.Size == 0)
        io.Fonts->AddFontDefault();

    static const ImWchar ranges[] = {
        (ImWchar)ICON_MIN_VS, (ImWchar)ICON_MAX_VS, 0
    };

    ImFontConfig cfg;
    cfg.MergeMode        = true;
    cfg.PixelSnapH       = true;
    cfg.GlyphMinAdvanceX = size;
    cfg.GlyphMaxAdvanceX = size;
    cfg.GlyphOffset      = ImVec2(0.0f, 3.0f); // Empurra os ícones para baixo para alinhar na titlebar

    s_iconFont = io.Fonts->AddFontFromFileTTF(ttfPath, size, &cfg, ranges);
}

// ── Backdrop ───────────────────────────────────────────────────────────────
// Replicates ImHex's AddShadowCircle(pos, diameter/2, ButtonActive@0.8, diameter/4, {0,0})
// Since our ImGui doesn't have AddShadowCircle, we draw:
//   1) A filled circle (solid shadow color)
//   2) A gradient ring fading from solid to transparent (using per-vertex colors)
void drawBackDrop() {
    if (!showTitleBarBackDrop) return;

    ImDrawList*    dl  = ImGui::GetWindowDrawList();
    ImGuiViewport* vp  = ImGui::GetMainViewport();

    const float diameter = 800.0f;
    const float radius   = diameter * 0.5f;
    const float spread   = diameter * 0.25f;
    const float outerR   = radius + spread;

    ImVec2 center(vp->Pos.x, vp->Pos.y - radius);

    ImVec4 baseCol = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
    ImU32 colSolid = ImGui::GetColorU32(ImVec4(baseCol.x, baseCol.y, baseCol.z, baseCol.w * backdropAlpha));
    ImU32 colFade  = ImGui::GetColorU32(ImVec4(baseCol.x, baseCol.y, baseCol.z, 0.0f));

    const int segs = 72;
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;

    // 1) Solid inner circle
    dl->AddCircleFilled(center, radius, colSolid, segs);

    // 2) Gradient ring: inner vertices = colSolid, outer vertices = transparent
    const int vtx_count = (segs + 1) * 2;
    const int idx_count = segs * 6;
    dl->PrimReserve(idx_count, vtx_count);

    ImDrawVert* vtx_write = dl->_VtxWritePtr;
    ImDrawIdx*  idx_write = dl->_IdxWritePtr;
    ImDrawIdx   vtx_base  = (ImDrawIdx)dl->_VtxCurrentIdx;

    for (int i = 0; i <= segs; i++) {
        float angle = (float)i / (float)segs * IM_PI * 2.0f;
        float cs = ImCos(angle), sn = ImSin(angle);

        vtx_write[i * 2 + 0].pos = ImVec2(center.x + cs * radius, center.y + sn * radius);
        vtx_write[i * 2 + 0].uv  = uv;
        vtx_write[i * 2 + 0].col = colSolid;

        vtx_write[i * 2 + 1].pos = ImVec2(center.x + cs * outerR, center.y + sn * outerR);
        vtx_write[i * 2 + 1].uv  = uv;
        vtx_write[i * 2 + 1].col = colFade;
    }

    for (int i = 0; i < segs; i++) {
        ImDrawIdx i0 = vtx_base + (ImDrawIdx)(i * 2);
        ImDrawIdx i1 = vtx_base + (ImDrawIdx)(i * 2 + 1);
        ImDrawIdx i2 = vtx_base + (ImDrawIdx)((i + 1) * 2);
        ImDrawIdx i3 = vtx_base + (ImDrawIdx)((i + 1) * 2 + 1);

        idx_write[i * 6 + 0] = i0;
        idx_write[i * 6 + 1] = i2;
        idx_write[i * 6 + 2] = i1;
        idx_write[i * 6 + 3] = i1;
        idx_write[i * 6 + 4] = i2;
        idx_write[i * 6 + 5] = i3;
    }

    dl->_VtxWritePtr   += vtx_count;
    dl->_IdxWritePtr   += idx_count;
    dl->_VtxCurrentIdx += vtx_count;
}

// ── draw() ─────────────────────────────────────────────────────────────────
// ATENÇÃO: chamar DEPOIS dos menu items, nunca antes.
// No ImHex, drawTitleBar() é chamado numa 3ª chamada a BeginMainMenuBar,
// após os menu items já terem sido desenhados.
bool draw(GLFWwindow* window, const std::string& title, bool borderless) {
    bool wantClose = false;

    float titleBarHeight = ImGui::GetCurrentWindowRead()->MenuBarHeight;

#if defined(GOWTOOL_OS_MACOS)
    // 1:1 ImHex: on macOS, effective titlebar height is 70% of MenuBarHeight
    titleBarHeight *= 0.7f;
    const float titlePosY = ImGui::GetStyle().FramePadding.y * 2.0f;

    // macOS NATIVE decorations: centered title, no custom buttons
    if (!borderless) {
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const ImVec2 textSize   = ImGui::CalcTextSize(title.c_str());
        float posX = (windowSize.x - textSize.x) * 0.5f;
        if (!NativeWindow::macosIsFullScreen(window))
            posX = ImMax(posX, 80.0f);
        ImGui::SetCursorPos(ImVec2(posX, titlePosY));
        ImGui::TextUnformatted(title.c_str());
        return false;
    }

    // macOS BORDERLESS: traffic lights from GLFW_DECORATED=TRUE, centered title only
    {
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const ImVec2 textSize   = ImGui::CalcTextSize(title.c_str());
        float posX = (windowSize.x - textSize.x) * 0.5f;
        if (!NativeWindow::macosIsFullScreen(window))
            posX = ImMax(posX, 80.0f);
        posX = ImMin(posX, windowSize.x - textSize.x - 8.0f);
        ImGui::SetCursorPos(ImVec2(posX, titlePosY));
        ImGui::TextUnformatted(title.c_str());
        return false;
    }

#else
    // Windows / Linux: draw custom min/max/close buttons only if borderless
    if (!borderless) return false;

    const ImVec2 buttonSize(titleBarHeight * 1.5f, titleBarHeight - 1.0f);
    const ImVec2 windowSize = ImGui::GetWindowSize();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button,        ImGui::GetColorU32(ImGuiCol_MenuBarBg));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabHovered));

    // 1:1 ImHex: buttons at Y=0
    ImGui::SetCursorPosX(windowSize.x - buttonSize.x * 3.0f);
    ImGui::SetCursorPosY(0.0f);

    if (TitleBarButton(ICON_VS_CHROME_MINIMIZE, buttonSize))
        glfwIconifyWindow(window);

    const bool maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
    if (maximized) {
        if (TitleBarButton(ICON_VS_CHROME_RESTORE, buttonSize))
            glfwRestoreWindow(window);
    } else {
        if (TitleBarButton(ICON_VS_CHROME_MAXIMIZE, buttonSize))
            glfwMaximizeWindow(window);
    }

    // 1:1 ImHex: close button colors (ABGR order in ImHex: 0xFF7A70F1 / 0xFF2311E8)
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(0xF1, 0x70, 0x7A, 0xFF));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0xE8, 0x11, 0x23, 0xFF));
    if (TitleBarButton(ICON_VS_CHROME_CLOSE, buttonSize))
        wantClose = true;
    ImGui::PopStyleColor(2);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // 1:1 ImHex: centered title at Y=0 (CursorPos = searchBoxPos where Y=0)
    {
        const ImVec2 textSize  = ImGui::CalcTextSize(title.c_str());
        const float  safeRight = windowSize.x - buttonSize.x * 3.0f - 4.0f;
        float posX = (windowSize.x - textSize.x) * 0.5f;
        posX = ImMin(posX, safeRight - textSize.x);
        posX = ImMax(posX, 8.0f);
        ImGui::SetCursorPos(ImVec2(posX, 0.0f));
        ImGui::TextUnformatted(title.c_str());
    }

    return wantClose;
#endif
}

} // namespace TitleBar
