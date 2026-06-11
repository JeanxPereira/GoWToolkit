#pragma once
#include <glm/glm.hpp>

namespace Onyx {

struct BoundingBox {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    glm::vec3 Center() const { return (min + max) * 0.5f; }
    float Radius() const { return glm::length(max - min) * 0.5f; }
};

} // namespace Onyx
