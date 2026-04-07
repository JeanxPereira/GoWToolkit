#include "Camera.h"
#include <algorithm>
#include <cmath>

namespace GOW {

Camera::Camera() {
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

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void Camera::ProcessMouseDrag(float dx, float dy) {
    m_yaw   -= dx * 0.005f;
    m_pitch += dy * 0.005f;
    // Clamp pitch to avoid gimbal lock
    m_pitch = std::clamp(m_pitch, -1.5f, 1.5f);
    UpdatePosition();
}

void Camera::ProcessMousePan(float dx, float dy) {
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

void Camera::FocusOn(const BoundingBox& bbox) {
    m_target = bbox.Center();
    m_distance = bbox.Radius() * 3.0f;
    if (m_distance < 1.0f) m_distance = 5.0f;
    m_yaw = glm::radians(45.0f);
    m_pitch = glm::radians(15.0f);
    UpdatePosition();
}

void Camera::Reset() {
    m_target = glm::vec3(0.0f);
    m_distance = 5.0f;
    m_yaw = glm::radians(45.0f);
    m_pitch = glm::radians(15.0f);
    UpdatePosition();
}

} // namespace GOW
