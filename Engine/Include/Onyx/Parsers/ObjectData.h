#pragma once
#include <algorithm>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace Onyx::Parsers {

/// A single joint in the skeleton hierarchy.
struct Joint {
  std::string name;

  // Hierarchy
  int16_t id = -1;
  int16_t parent = -1; // -1 = root
  int16_t childsStart = -1;
  int16_t childsEnd = -1;
  int16_t externalId = -1;
  int16_t invId = -1; // index into Matrixes3 (inverse bind pose)

  // Flags
  uint32_t flags = 0;
  bool isSkinned = false;    // flag & 0x80
  bool isExternal = false;   // flag & 0x08
  bool isQuaternion = false; // flag & 0x8000 (GOW1 only; GOW2 always false → Euler)
  float unkCoeff = 0.0f;

  // Computed by FillJoints() — final ready-to-use matrices
  glm::mat4 parentToJoint{1.0f};  // local transform (parent space → this joint)
  glm::mat4 bindToJointMat{1.0f}; // inverse bind pose (world → joint local)
  glm::mat4 renderMat{1.0f}; // world-space rest pose (for debug visualization)
};

/// Skeleton + bind pose data parsed from an Object tag.
/// Produced by ObjectParser, consumed by Viewport3D/SceneRenderer.
struct ObjectData {
  std::vector<Joint> joints;

  // Raw matrix arrays (indices match joints via joint.invId for Matrixes3)
  std::vector<glm::mat4> matrixes1; // parent→joint local transform per joint
  std::vector<glm::mat4> matrixes2; // external rotation matrices
  std::vector<glm::mat4> matrixes3; // inverse bind pose per skinned joint

  // Idle pose (one per joint)
  std::vector<glm::vec4> vectors4;  // idle local position
  std::vector<glm::ivec4> vectors5; // idle rotation (Q.14 fixed-point)
  std::vector<glm::vec4> vectors6;  // idle scale
  std::vector<glm::vec4> vectors7;  // unknown (zero in most cases)

  // Internal tracking
  uint32_t file0x20 = 0;
  uint32_t file0x24 = 0;

  bool HasSkeleton() const { return !joints.empty(); }
};

} // namespace Onyx::Parsers
