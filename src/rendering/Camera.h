#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace GOW {

struct BoundingBox {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    glm::vec3 Center() const { return (min + max) * 0.5f; }
    float Radius() const { return glm::length(max - min) * 0.5f; }
};

class Camera {
public:
    Camera();

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

    void ProcessMouseDrag(float dx, float dy);
    void ProcessMousePan(float dx, float dy);
    void ProcessScroll(float delta);
    void FocusOn(const BoundingBox& bbox);
    void Reset();

    // Configurable
    float fov     = 55.0f;
    float nearPlane = 0.01f;
    float farPlane  = 50000.0f;

private:
    void UpdatePosition();

    glm::vec3 m_target{0.0f, 0.0f, 0.0f};
    float m_distance = 5.0f;
    float m_yaw   = glm::radians(45.0f);   // 1/4 isometric view (right side)
    float m_pitch = glm::radians(15.0f);   // slight elevation

    glm::vec3 m_position{0.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
};

} // namespace GOW
