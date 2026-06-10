#pragma once
#include "core/parsers/shared/AnimationData.h"
#include "core/parsers/shared/ObjectData.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Onyx {

// Per-frame pose cache for one act. Built once via Bake() and queried with
// EvaluateAt() so timeline scrubbing is O(1) regardless of how additive the
// stream is. Stored SoA so a frame is jointCount consecutive vec4 entries.
struct BakedAnimation {
    int   frameCount = 0;
    int   jointCount = 0;
    float frameTime  = 1.0f / 30.0f;
    float duration   = 0.0f;

    std::vector<glm::vec4> rotations;  // frameCount * jointCount
    std::vector<glm::vec4> positions;  // frameCount * jointCount
    std::vector<glm::vec4> scales;     // frameCount * jointCount

    bool empty() const { return frameCount == 0 || jointCount == 0; }
};

class AnimationPlayer {
public:
    AnimationPlayer() = default;

    void SetAnimation(const AnimationData* anim, int groupIdx, int actIdx,
                      const ObjectData* skeleton);

    bool Update(float dt);
    void Reset();
    void Stop();

    bool IsPlaying() const { return m_playing; }
    void SetPlaying(bool p) { m_playing = p; }
    void Toggle() { m_playing = !m_playing; }

    enum class LoopMode { None = 0, Loop = 1, PingPong = 2 };
    LoopMode GetLoopMode() const { return m_loopMode; }
    void SetLoopMode(LoopMode m) { m_loopMode = m; }
    // Back-compat shim for callers that still treat looping as a bool.
    bool IsLooping() const { return m_loopMode != LoopMode::None; }
    void SetLooping(bool loop) { m_loopMode = loop ? LoopMode::Loop : LoopMode::None; }

    float GetTime() const { return m_time; }
    float GetDuration() const { return m_currentAct ? m_currentAct->duration : 0.0f; }
    void  SetTime(float t);

    float GetSpeed() const { return m_speed; }
    void  SetSpeed(float s) { m_speed = s; }

    // Frame-domain helpers built on the baked cache.
    int   GetFrameCount() const { return m_currentBaked ? m_currentBaked->frameCount : 0; }
    float GetFrameTime() const  { return m_currentBaked ? m_currentBaked->frameTime  : (1.0f/30.0f); }
    int   GetCurrentFrame() const;
    void  SetFrame(int f);

    const std::string& GetCurrentGroupName() const { return m_currentGroupName; }
    const std::string& GetCurrentActName() const { return m_currentActName; }
    int GetCurrentGroupIndex() const { return m_currentGroupIdx; }
    int GetCurrentActIndex() const { return m_currentActIdx; }

    const std::vector<glm::vec4>& GetJointPositions() const { return m_jointLocalPos; }
    const std::vector<glm::vec4>& GetJointRotations() const { return m_jointLocalRot; }
    const std::vector<glm::vec4>& GetJointScales() const { return m_jointLocalScale; }

    std::vector<glm::mat4> ComputeJointMatrices() const;

    // Access to the baked cache for UI (timeline markers, dopesheet, curves).
    const BakedAnimation* GetBaked() const { return m_currentBaked; }

private:
    bool HandleSkinningStream(const AnimSubstream& stream,
                              const AnimSamplesManager& globalMgr,
                              float prevTime, float newTime,
                              float frameTime,
                              std::vector<glm::vec4>& jointData);

    float ReturnStreamDataIndex(const AnimSamplesManager& manager,
                                const AnimSamplesManager& globalMgr,
                                float animTime, float frameTime) const;

    // Bake the current act if not already cached. Returns the cache entry
    // or nullptr if baking failed (no skinning data, no skeleton, etc.).
    BakedAnimation* EnsureBaked();
    // Sample baked cache at `time` and update m_jointLocal{Pos,Rot,Scale}.
    void ApplyBakedAt(float time);

    struct BakeKey {
        const AnimationData* anim;
        int group;
        int act;
        bool operator==(const BakeKey& o) const {
            return anim == o.anim && group == o.group && act == o.act;
        }
    };
    struct BakeKeyHash {
        size_t operator()(const BakeKey& k) const noexcept {
            // FNV-1a-ish mix; fine for cache keying.
            size_t h = reinterpret_cast<size_t>(k.anim);
            h ^= (size_t)k.group * 0x9E3779B97F4A7C15ull;
            h ^= (size_t)k.act   * 0xBF58476D1CE4E5B9ull + (h << 6) + (h >> 2);
            return h;
        }
    };
    std::unordered_map<BakeKey, std::unique_ptr<BakedAnimation>, BakeKeyHash> m_bakeCache;
    BakedAnimation* m_currentBaked = nullptr;

    const AnimationData* m_anim = nullptr;
    const AnimAct*       m_currentAct = nullptr;
    const ObjectData*    m_skeleton = nullptr;
    int                  m_stateIndex = -1;
    int                  m_currentGroupIdx = -1;
    int                  m_currentActIdx = -1;
    std::string          m_currentGroupName;
    std::string          m_currentActName;
    float                m_time = 0.0f;
    float                m_speed = 1.0f;
    bool                 m_playing = false;
    LoopMode             m_loopMode = LoopMode::Loop;
    int                  m_pingPongDir = 1; // +1 forward, -1 backward (PingPong only)

    std::vector<glm::vec4> m_jointLocalPos;
    std::vector<glm::vec4> m_jointLocalRot;
    std::vector<glm::vec4> m_jointLocalScale;
};

} // namespace Onyx
