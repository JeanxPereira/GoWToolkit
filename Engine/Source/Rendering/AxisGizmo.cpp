#include <Onyx/Rendering/AxisGizmo.h>
#include <algorithm>

namespace Onyx::Rendering {

namespace {

struct AxisInfo {
    CameraView view;
    glm::vec3  worldDir;
    ImU32      color;
    const char* label;
    bool       positive;
};

// Axis order arbitrary; identified by index at render time.
//
// Color convention: X = red, Y = green, Z = blue (Blender / Maya).
//
// Mapping: a disc sitting on +X in world space, when clicked, jumps the camera
// to the +X side (= Right view), so it visibly aligns "this disc faces you".
constexpr ImU32 kColX = IM_COL32(231,  75,  96, 255);
constexpr ImU32 kColY = IM_COL32(139, 199,  51, 255);
constexpr ImU32 kColZ = IM_COL32( 44, 137, 226, 255);

const AxisInfo kAxes[6] = {
    { CameraView::Right,  { 1.0f, 0.0f, 0.0f}, kColX, "X", true  },
    { CameraView::Left,   {-1.0f, 0.0f, 0.0f}, kColX, "X", false },
    { CameraView::Top,    { 0.0f, 1.0f, 0.0f}, kColY, "Y", true  },
    { CameraView::Bottom, { 0.0f,-1.0f, 0.0f}, kColY, "Y", false },
    { CameraView::Front,  { 0.0f, 0.0f, 1.0f}, kColZ, "Z", true  },
    { CameraView::Back,   { 0.0f, 0.0f,-1.0f}, kColZ, "Z", false },
};

constexpr float kMargin     = 16.0f; // gap between gizmo and viewport edge
constexpr float kRadius     = 38.0f; // distance from gizmo center to axis tips
constexpr float kDiscRadius = 10.0f; // axis-tip disc radius
constexpr float kHoverGrow  = 1.20f; // hover scale

inline ImU32 WithAlpha(ImU32 col, unsigned alpha) {
    return (col & 0x00FFFFFFu) | ((alpha & 0xFFu) << 24);
}

} // namespace

bool AxisGizmo::Draw(const glm::mat3& viewRot,
                     const ImVec2& /*imageMin*/,
                     const ImVec2& imageMax,
                     CameraView& outView) {
    // Bottom-right anchor.
    const ImVec2 center(imageMax.x - kMargin - kRadius,
                        imageMax.y - kMargin - kRadius);

    // Project each world axis through the view rotation. With glm::lookAt the
    // forward direction is -Z, so a point with viewSpace.z < 0 is in front of
    // the camera (closer to viewer when drawn). screen-Y is flipped vs. world-Y.
    struct Entry { ImVec2 pos; float depth; };
    Entry entries[6];
    for (int i = 0; i < 6; ++i) {
        glm::vec3 vs = viewRot * kAxes[i].worldDir;
        entries[i].pos   = ImVec2(center.x + vs.x * kRadius,
                                  center.y - vs.y * kRadius);
        entries[i].depth = vs.z;
    }

    // Front-to-back hit test (ascending depth = closer to viewer first).
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    int hoveredIdx = -1;
    {
        int order[6] = {0,1,2,3,4,5};
        std::sort(order, order + 6, [&](int a, int b) {
            return entries[a].depth < entries[b].depth;
        });
        for (int k = 0; k < 6; ++k) {
            int i = order[k];
            float dx = mouse.x - entries[i].pos.x;
            float dy = mouse.y - entries[i].pos.y;
            float rr = kDiscRadius * (k == 0 ? kHoverGrow : 1.0f);
            if (dx * dx + dy * dy <= rr * rr) {
                hoveredIdx = i;
                break;
            }
        }
    }
    m_hovered = (hoveredIdx >= 0);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Lines from center to each positive axis tip, drawn back-to-front so a
    // closer line overpaints a farther one when they share a screen region.
    {
        int posIdx[3]; int pc = 0;
        for (int i = 0; i < 6; ++i) if (kAxes[i].positive) posIdx[pc++] = i;
        std::sort(posIdx, posIdx + 3, [&](int a, int b) {
            return entries[a].depth > entries[b].depth;
        });
        for (int k = 0; k < 3; ++k) {
            int i = posIdx[k];
            dl->AddLine(center, entries[i].pos, kAxes[i].color, 2.0f);
        }
    }

    // Discs back-to-front.
    int order[6] = {0,1,2,3,4,5};
    std::sort(order, order + 6, [&](int a, int b) {
        return entries[a].depth > entries[b].depth;
    });
    for (int k = 0; k < 6; ++k) {
        int i = order[k];
        const AxisInfo& a = kAxes[i];
        const float radius = kDiscRadius * ((i == hoveredIdx) ? kHoverGrow : 1.0f);
        if (a.positive) {
            dl->AddCircleFilled(entries[i].pos, radius, a.color, 20);
            dl->AddCircle(entries[i].pos, radius, IM_COL32(0, 0, 0, 96), 20, 1.0f);
            ImVec2 sz = ImGui::CalcTextSize(a.label);
            ImVec2 textPos(entries[i].pos.x - sz.x * 0.5f,
                           entries[i].pos.y - sz.y * 0.5f);
            dl->AddText(textPos, IM_COL32(255, 255, 255, 255), a.label);
        } else {
            dl->AddCircleFilled(entries[i].pos, radius, WithAlpha(a.color, 70), 20);
            dl->AddCircle(entries[i].pos, radius, a.color, 20, 1.5f);
        }
    }

    if (hoveredIdx >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        outView = kAxes[hoveredIdx].view;
        return true;
    }
    return false;
}

} // namespace Onyx::Rendering
