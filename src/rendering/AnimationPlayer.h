#pragma once
#include "core/parsers/shared/AnimationData.h"
#include "core/parsers/shared/ObjectData.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace GOW {

/// Runtime animation player — evaluates skeletal animation per frame.
/// Port of AnimationObjectSkelet from the Go browser's Animation.js.
class AnimationPlayer {
public:
    AnimationPlayer() = default;

    /// Set up a specific animation act for playback.
    /// skeleton must remain valid for the lifetime of the player.
    void SetAnimation(const AnimationData* anim, int groupIdx, int actIdx,
                      const ObjectData* skeleton);

    /// Advance animation by dt seconds. Returns true if joint data changed.
    bool Update(float dt);

    /// Reset to idle pose (T-pose from skeleton).
    void Reset();

    /// Stop playback and reset.
    void Stop();

    bool IsPlaying() const { return m_playing; }
    bool IsLooping() const { return m_loop; }
    void SetLooping(bool loop) { m_loop = loop; }

    float GetTime() const { return m_time; }
    float GetDuration() const { return m_currentAct ? m_currentAct->duration : 0.0f; }
    void SetTime(float t);  // Seek to specific time

    const std::string& GetCurrentGroupName() const { return m_currentGroupName; }
    const std::string& GetCurrentActName() const { return m_currentActName; }
    int GetCurrentGroupIndex() const { return m_currentGroupIdx; }
    int GetCurrentActIndex() const { return m_currentActIdx; }

    /// Get current animated joint local transforms.
    /// These are in Q.14 fixed-point for rotations (matching ObjectData::vectors5).
    const std::vector<glm::vec4>&  GetJointPositions() const { return m_jointLocalPos; }
    const std::vector<glm::vec4>&  GetJointRotations() const { return m_jointLocalRot; }
    const std::vector<glm::vec4>&  GetJointScales() const { return m_jointLocalScale; }

    /// Compute world-space joint matrices for the current animation pose.
    /// Returns one mat4 per joint — these replace the idle-pose matrices.
    std::vector<glm::mat4> ComputeJointMatrices() const;

private:
    /// Process a single skinning stream (rotation or position).
    /// Returns true if any values were modified.
    bool HandleSkinningStream(const AnimSubstream& stream,
                              const AnimSamplesManager& globalMgr,
                              float prevTime, float newTime,
                              float frameTime,
                              std::vector<glm::vec4>& jointData);

    /// Get sample index within a stream for a given time.
    /// Returns -1 if out of range.
    float ReturnStreamDataIndex(const AnimSamplesManager& manager,
                                const AnimSamplesManager& globalMgr,
                                float animTime, float frameTime) const;

    const AnimationData* m_anim = nullptr;
    const AnimAct*       m_currentAct = nullptr;
    const ObjectData*    m_skeleton = nullptr;
    int                  m_stateIndex = -1;
    int                  m_currentGroupIdx = -1;
    int                  m_currentActIdx = -1;
    std::string          m_currentGroupName;
    std::string          m_currentActName;
    float                m_time = 0.0f;
    bool                 m_playing = false;
    bool                 m_loop = true;

    // Current pose (initialized from skeleton idle vectors 4/5/6)
    std::vector<glm::vec4> m_jointLocalPos;
    std::vector<glm::vec4> m_jointLocalRot;    // float version of Q.14
    std::vector<glm::vec4> m_jointLocalScale;
};

} // namespace GOW
