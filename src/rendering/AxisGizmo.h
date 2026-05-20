#pragma once
#include "Camera.h"
#include <glm/glm.hpp>
#include <imgui.h>

namespace GOW {

// Blender-style navigation gizmo. Draws six axis discs (+X/-X/+Y/-Y/+Z/-Z)
// in the bottom-right corner of the viewport using ImDrawList (no GL state).
// On left-click, fires a snap-to-view request via the out parameter.
class AxisGizmo {
public:
    // Draw and hit-test in one call. Must be invoked after ImGui::Image()
    // (the FBO display) so the call has the image rect for placement.
    //
    //   viewRot   : glm::mat3(view) — rotation portion of the camera's lookAt
    //   imageMin  : top-left of the FBO Image rect (screen-space, ImGui units)
    //   imageMax  : bottom-right of the FBO Image rect
    //   outView   : on click, set to the requested canonical view
    //   returns   : true iff the user clicked an axis disc this frame
    bool Draw(const glm::mat3& viewRot,
              const ImVec2& imageMin,
              const ImVec2& imageMax,
              CameraView& outView);

    // True iff the mouse is currently over the gizmo footprint. Use this in
    // the viewport input handler to suppress orbit/pan/zoom while the user
    // is interacting with the widget.
    bool IsHovered() const { return m_hovered; }

private:
    bool m_hovered = false;
};

} // namespace GOW
