#include "Ui/CameraPanel.h"
#include <Onyx/Viewers/Viewport3D.h>
#include <Onyx/Viewers/IDocumentContent.h>
#include <Onyx/Rendering/Camera.h>
#include <Onyx/Services/AppConfig.h>
#include <Onyx/Api/ToolkitApi.h>
#include <Onyx/Viewers/DocumentWindow.h>
#include "imgui.h"

namespace Onyx::App {

CameraPanel* CameraPanel::s_instance = nullptr;

CameraPanel::CameraPanel() {
    s_instance = this;
    if (auto* cfg = Onyx::Services::AppConfig::Get()) {
        visible = cfg->camPanelVisible;
    } else {
        visible = false;
    }
}

CameraPanel::~CameraPanel() {
    if (s_instance == this) s_instance = nullptr;
}

void CameraPanel::Toggle() {
    if (s_instance) {
        s_instance->visible = !s_instance->visible;
    }
}

void CameraPanel::Draw() {
    Onyx::Services::AppConfig* cfg = Onyx::Services::AppConfig::Get();

    // Persist visibility every frame: View menu and the viewport "Cam" button
    // both drive IPanel::visible (via CameraPanel::Toggle()), so the panel
    // mirrors that into the config so it survives restart.
    if (cfg) cfg->camPanelVisible = visible;

    if (!visible) return;

    if (!ImGui::Begin("Camera", &visible, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        if (cfg) cfg->camPanelVisible = visible;
        return;
    }

    // Resolve the active viewport via IDocumentContent::GetEmbeddedViewport,
    // so both bare Viewport3D documents (Object viewer) and composite ones
    // (MapViewer, which wraps a Viewport3D internally) bind correctly.
    Onyx::Viewers::Viewport3D* vp = nullptr;
    if (Onyx::Api::Documents().HasActiveDocument()) {
        auto doc = Onyx::Api::Documents().GetActiveDocument();
        if (doc) vp = doc->GetEmbeddedViewport();
    }

    if (!vp) {
        ImGui::TextDisabled("No active 3D viewport.");
        ImGui::End();
        if (cfg) cfg->camPanelVisible = visible;
        return;
    }

    Onyx::Rendering::Camera& cam = vp->GetCamera();
    bool changed = false;

    // â”€â”€ FOV â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (ImGui::SliderFloat("FOV", &cam.fov, 10.0f, 120.0f, "%.1f")) {
        changed = true;
    }

    ImGui::Separator();
    ImGui::TextDisabled("Auto adapts near/far based on scene bounds and orbit distance. Disable to force fixed planes.");

    // â”€â”€ Near plane â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (ImGui::Checkbox("Auto near", &cam.autoNear)) changed = true;
    ImGui::BeginDisabled(cam.autoNear);
    if (ImGui::DragFloat("Manual near", &cam.manualNear, 0.01f, 0.001f, 10000.0f, "%.3f")) {
        changed = true;
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!cam.autoNear);
    if (ImGui::DragFloat("Near distance scale", &cam.nearDistanceScale, 0.0001f, 0.0001f, 1.0f, "%.4f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("near = distance * scale (auto mode)");
    if (ImGui::DragFloat("Near/far ratio max", &cam.nearFarRatioMax, 100.0f, 100.0f, 1.0e6f, "%.0f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lower bound for near = far/ratio. Smaller = tighter slab, less Z-fighting.");
    if (ImGui::DragFloat("Near hard floor", &cam.nearPlane, 0.001f, 0.0001f, 100.0f, "%.4f")) {
        changed = true;
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    // â”€â”€ Far plane â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (ImGui::Checkbox("Auto far", &cam.autoFar)) changed = true;
    ImGui::BeginDisabled(cam.autoFar);
    if (ImGui::DragFloat("Manual far", &cam.manualFar, 10.0f, 1.0f, 1.0e6f, "%.1f")) {
        changed = true;
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!cam.autoFar);
    if (ImGui::DragFloat("Far margin", &cam.farMargin, 0.01f, 1.0f, 10.0f, "%.2f")) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("far = farthest_bbox_corner_distance * margin");
    if (ImGui::DragFloat("Far hard cap", &cam.farPlane, 100.0f, 100.0f, 1.0e7f, "%.0f")) {
        changed = true;
    }
    ImGui::EndDisabled();

    // â”€â”€ Diagnostics â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    ImGui::Separator();
    int w = vp->GetFboWidth(), h = vp->GetFboHeight();
    float aspect = (h > 0) ? (float)w / (float)h : 1.0f;
    float effN = 0.0f, effF = 0.0f;
    cam.GetEffectivePlanes(aspect, effN, effF);
    ImGui::Text("Effective near: %.3f", effN);
    ImGui::Text("Effective far : %.1f", effF);
    ImGui::Text("Far / near    : %.0f", effN > 0.0f ? effF / effN : 0.0f);
    ImGui::Text("Orbit dist    : %.1f", cam.GetDistance());
    glm::vec3 mn = cam.GetSceneMin();
    glm::vec3 mx = cam.GetSceneMax();
    ImGui::Text("Scene min     : %.1f %.1f %.1f", mn.x, mn.y, mn.z);
    ImGui::Text("Scene max     : %.1f %.1f %.1f", mx.x, mx.y, mx.z);
    ImGui::Text("Scene radius  : %.1f", cam.GetSceneRadius());

    // â”€â”€ Reset â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    ImGui::Separator();
    if (ImGui::Button("Reset to defaults")) {
        cam.fov               = 55.0f;
        cam.nearPlane         = 0.01f;
        cam.farPlane          = 50000.0f;
        cam.autoNear          = true;
        cam.autoFar           = true;
        cam.manualNear        = 0.1f;
        cam.manualFar         = 10000.0f;
        cam.nearDistanceScale = 0.002f;
        cam.farMargin         = 1.1f;
        cam.nearFarRatioMax   = 50000.0f;
        changed = true;
    }

    if (changed) {
        vp->RequestRedraw();
        if (cfg) {
            cfg->camFov               = cam.fov;
            cfg->camNearPlane         = cam.nearPlane;
            cfg->camFarPlane          = cam.farPlane;
            cfg->camAutoNear          = cam.autoNear;
            cfg->camAutoFar           = cam.autoFar;
            cfg->camManualNear        = cam.manualNear;
            cfg->camManualFar         = cam.manualFar;
            cfg->camNearDistanceScale = cam.nearDistanceScale;
            cfg->camFarMargin         = cam.farMargin;
            cfg->camNearFarRatioMax   = cam.nearFarRatioMax;
        }
    }

    // Mirror visibility flips (e.g. user closes via window X button).
    if (cfg) cfg->camPanelVisible = visible;

    ImGui::End();
}

} // namespace Onyx::App
