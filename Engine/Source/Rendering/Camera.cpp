#include <Onyx/Rendering/Camera.h>
#include <Onyx/Services/AppConfig.h>
#include <algorithm>
#include <cmath>

namespace Onyx::Rendering {

Camera::Camera() {
    // Pull persisted tuning knobs so the projection slab matches the user's
    // last-session preferences immediately (no per-viewport sync needed).
    if (auto* cfg = Onyx::Services::AppConfig::Get()) {
        fov               = cfg->camFov;
        nearPlane         = cfg->camNearPlane;
        farPlane          = cfg->camFarPlane;
        autoNear          = cfg->camAutoNear;
        autoFar           = cfg->camAutoFar;
        manualNear        = cfg->camManualNear;
        manualFar         = cfg->camManualFar;
        nearDistanceScale = cfg->camNearDistanceScale;
        farMargin         = cfg->camFarMargin;
        nearFarRatioMax   = cfg->camNearFarRatioMax;
    }
    UpdatePosition();
}

void Camera::UpdatePosition() {
    float x = m_distance * cosf(m_pitch) * sinf(m_yaw);
    float y = m_distance * sinf(m_pitch);
    float z = m_distance * cosf(m_pitch) * cosf(m_yaw);
    m_position = m_target + glm::vec3(x, y, z);
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, m_target, m_up);
}

glm::mat3 Camera::GetViewRotation() const {
    return glm::mat3(GetViewMatrix());
}

void Camera::GetEffectivePlanes(float /*aspect*/, float& outNear, float& outFar) const {
    // Far plane = distance from camera position to the farthest scene bbox
    // corner, so the whole map (geometry + sky) always renders no matter
    // how the user pans, rotates or zooms. Near plane scales with the orbit
    // distance to keep the near/far ratio low â€” preserving depth precision
    // and avoiding Z-fighting that plagued large CXT scenes with the
    // original fixed 0.01..50000 slab.
    float f, n;

    if (autoFar) {
        float farDist = 0.0f;
        for (int i = 0; i < 8; ++i) {
            glm::vec3 corner(
                (i & 1) ? m_sceneMax.x : m_sceneMin.x,
                (i & 2) ? m_sceneMax.y : m_sceneMin.y,
                (i & 4) ? m_sceneMax.z : m_sceneMin.z);
            farDist = std::max(farDist, glm::length(corner - m_position));
        }
        f = std::max(farDist * farMargin, m_distance * 2.0f);
        f = std::min(f, farPlane);
    } else {
        f = manualFar;
    }

    if (autoNear) {
        n = std::max(m_distance * nearDistanceScale, f / std::max(nearFarRatioMax, 1.0f));
        n = std::max(n, nearPlane);
    } else {
        n = manualNear;
    }

    // Guard against degenerate slabs.
    if (n <= 0.0f) n = nearPlane;
    if (f <= n)    f = n * 10.0f;

    outNear = n;
    outFar  = f;
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
    float n, f;
    GetEffectivePlanes(aspect, n, f);
    return glm::perspective(glm::radians(fov), aspect, n, f);
}

void Camera::ProcessMouseDrag(float dx, float dy) {
    m_animActive = false;
    m_yaw   -= dx * 0.005f;
    m_pitch += dy * 0.005f;
    // Clamp pitch to avoid gimbal lock
    m_pitch = std::clamp(m_pitch, -1.5f, 1.5f);
    UpdatePosition();
}

void Camera::ProcessMousePan(float dx, float dy) {
    m_animActive = false;
    glm::vec3 forward = glm::normalize(m_target - m_position);
    glm::vec3 right   = glm::normalize(glm::cross(forward, m_up));
    glm::vec3 up      = glm::normalize(glm::cross(right, forward));

    float panSpeed = m_distance * 0.002f;
    m_target -= right * dx * panSpeed;
    m_target += up * dy * panSpeed;
    UpdatePosition();
}

void Camera::ProcessScroll(float delta) {
    // Use exponential zoom to handle large scroll deltas safely.
    // If delta is positive (scroll up/zoom in), distance multiplies by < 1.0 (gets closer).
    // If delta is negative (scroll down/zoom out), distance multiplies by > 1.0 (gets further).
    m_distance *= std::pow(0.9f, delta);
    m_distance = std::clamp(m_distance, 0.1f, 50000.0f);
    UpdatePosition();
}

void Camera::FocusOn(const Domain::BoundingBox& bbox) {
    m_animActive = false;
    m_target = bbox.Center();
    float r = bbox.Radius();
    m_sceneRadius = std::max(r, 1.0f);
    m_sceneMin = bbox.min;
    m_sceneMax = bbox.max;
    float halfFov = glm::radians(fov * 0.5f);
    m_distance = std::max(r / std::sin(halfFov) * 1.2f, 0.1f);
    m_yaw = glm::radians(45.0f);
    m_pitch = glm::radians(15.0f);
    UpdatePosition();
}

void Camera::Reset() {
    m_target = glm::vec3(0.0f);
    m_distance = 5.0f;
    m_yaw = glm::radians(45.0f);
    m_pitch = glm::radians(15.0f);
    m_animActive = false;
    UpdatePosition();
}

void Camera::SnapToView(CameraView v) {
    // Top/Bottom: keep current yaw (matches Blender's behaviour where the "up"
    // direction in a top view follows your previous orbit). Pitch is nudged off
    // the singularity at Â±Ï€/2 so cos(pitch) stays non-zero in UpdatePosition.
    constexpr float kPi    = 3.14159265358979323846f;
    constexpr float kEps   = 1e-3f;
    constexpr float kTopP  =  kPi * 0.5f - kEps;
    constexpr float kBotP  = -kPi * 0.5f + kEps;

    float targetYaw   = m_yaw;
    float targetPitch = m_pitch;
    switch (v) {
        case CameraView::Front:  targetYaw = 0.0f;          targetPitch = 0.0f;   break;
        case CameraView::Back:   targetYaw = kPi;           targetPitch = 0.0f;   break;
        case CameraView::Right:  targetYaw = kPi * 0.5f;    targetPitch = 0.0f;   break;
        case CameraView::Left:   targetYaw = -kPi * 0.5f;   targetPitch = 0.0f;   break;
        case CameraView::Top:    targetPitch = kTopP;                              break;
        case CameraView::Bottom: targetPitch = kBotP;                              break;
    }

    // Normalize yaw delta to (-Ï€, Ï€] so the tween always takes the short way.
    float deltaYaw = targetYaw - m_yaw;
    while (deltaYaw >  kPi) deltaYaw -= 2.0f * kPi;
    while (deltaYaw <= -kPi) deltaYaw += 2.0f * kPi;

    m_animFromYaw   = m_yaw;
    m_animFromPitch = m_pitch;
    m_animToYaw     = m_yaw + deltaYaw;
    m_animToPitch   = targetPitch;
    m_animTime      = 0.0f;
    m_animActive    = true;
}

bool Camera::UpdateAnimation(float dt) {
    if (!m_animActive) return false;

    m_animTime += dt;
    float t = (m_animDuration > 0.0f) ? (m_animTime / m_animDuration) : 1.0f;
    if (t >= 1.0f) {
        t = 1.0f;
        m_animActive = false;
    }
    // Smoothstep ease-in-out.
    float s = t * t * (3.0f - 2.0f * t);
    m_yaw   = m_animFromYaw   + (m_animToYaw   - m_animFromYaw)   * s;
    m_pitch = m_animFromPitch + (m_animToPitch - m_animFromPitch) * s;
    UpdatePosition();
    return true;
}

} // namespace Onyx::Rendering
