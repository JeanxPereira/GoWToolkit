#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/domain/BoundingBox.h"

namespace GOW {

enum class CameraView {
    Front,   // +Z
    Back,    // -Z
    Right,   // +X
    Left,    // -X
    Top,     // +Y
    Bottom,  // -Y
};

class Camera {
public:
    Camera();

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;
    glm::mat3 GetViewRotation() const;

    void ProcessMouseDrag(float dx, float dy);
    void ProcessMousePan(float dx, float dy);
    void ProcessScroll(float delta);
    void FocusOn(const BoundingBox& bbox);
    void Reset();

    // Animated snap to a canonical orientation (Front/Back/Right/Left/Top/Bottom).
    void SnapToView(CameraView v);
    // Advance any active snap tween. Returns true if camera state changed this tick.
    bool UpdateAnimation(float dt);
    bool IsAnimating() const { return m_animActive; }

    // Read-only diagnostics used by the camera widget so the user can see
    // the values the adaptive projection picked.
    void GetEffectivePlanes(float aspect, float& outNear, float& outFar) const;
    float GetDistance()  const { return m_distance; }
    glm::vec3 GetPosition()    const { return m_position; }
    glm::vec3 GetSceneMin()    const { return m_sceneMin; }
    glm::vec3 GetSceneMax()    const { return m_sceneMax; }
    float GetSceneRadius()     const { return m_sceneRadius; }

    // ── Tuning knobs (exposed to the in-viewport camera widget) ────────
    float fov     = 55.0f;
    float nearPlane = 0.01f;   // hard lower bound for near (auto or manual)
    float farPlane  = 50000.0f;// hard upper bound for far  (auto or manual)

    // Auto mode (default). When false, manualNear/manualFar are used as-is.
    bool  autoNear   = true;
    bool  autoFar    = true;
    float manualNear = 0.1f;
    float manualFar  = 10000.0f;

    // Auto-mode tuning:
    //   near = max(distance * nearDistanceScale, far / nearFarRatioMax)
    //   far  = max(farthest_bbox_corner_distance * farMargin, distance*2)
    float nearDistanceScale = 0.002f;
    float farMargin         = 1.1f;
    float nearFarRatioMax   = 50000.0f;

private:
    void UpdatePosition();

    glm::vec3 m_target{0.0f, 0.0f, 0.0f};
    float m_distance = 5.0f;
    float m_yaw   = glm::radians(45.0f);   // 1/4 isometric view (right side)
    float m_pitch = glm::radians(15.0f);   // slight elevation

    glm::vec3 m_position{0.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};

    // Scene extent captured at FocusOn time so the projection slab can size
    // its far plane around the actual geometry (not a fixed 50k).
    float m_sceneRadius = 5.0f;
    glm::vec3 m_sceneMin{-5.0f};
    glm::vec3 m_sceneMax{ 5.0f};

    // Snap-to-view tween state. SnapToView seeds the From/To pair; UpdateAnimation
    // ticks m_animTime forward and lerps yaw/pitch with smoothstep easing.
    bool  m_animActive   = false;
    float m_animTime     = 0.0f;
    float m_animDuration = 0.25f;
    float m_animFromYaw  = 0.0f;
    float m_animFromPitch= 0.0f;
    float m_animToYaw    = 0.0f;
    float m_animToPitch  = 0.0f;
};

} // namespace GOW
