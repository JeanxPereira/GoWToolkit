// AnimationPlayer — runtime skeletal animation evaluator
// Port of AnimationObjectSkelet from Animation.js in the Go browser.
//
// Decoding strategy: GOW2 streams are delta-encoded, so random-access seek
// requires replaying every sample from frame 0. We do that exactly once per
// act (Bake), store the resulting per-frame pose in a flat SoA cache, and
// turn SetTime/Update/SetFrame into O(1) lookups. The original step-driven
// HandleSkinningStream path is kept and reused for the bake walk.

#include "AnimationPlayer.h"
#include "core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace GOW {

static constexpr float kEps = 1.0f / (1024.0f * 16.0f);
static constexpr float kQuatToFloat = 1.0f / (1 << 14);

// ── SetAnimation ───────────────────────────────────────────────────────────

void AnimationPlayer::SetAnimation(const AnimationData* anim, int groupIdx, int actIdx,
                                    const ObjectData* skeleton) {
    if (!anim || !skeleton) return;
    if (groupIdx < 0 || groupIdx >= (int)anim->groups.size()) return;

    const auto& group = anim->groups[groupIdx];
    if (group.isExternal) return;
    if (actIdx < 0 || actIdx >= (int)group.acts.size()) return;

    m_anim = anim;
    m_skeleton = skeleton;
    m_currentAct = &group.acts[actIdx];
    m_currentGroupIdx = groupIdx;
    m_currentActIdx = actIdx;
    m_currentGroupName = group.name;
    m_currentActName = m_currentAct->name;
    m_stateIndex = anim->FindSkinningTypeIndex();
    m_currentBaked = nullptr;

    Reset();

    BakedAnimation* baked = EnsureBaked();
    if (baked) {
        ApplyBakedAt(0.0f);
        LOG_INFO("[AnimPlayer] Playing '%s: %s' duration=%.3fs frames=%d frameTime=%.4f stateIdx=%d",
                 m_currentGroupName.c_str(), m_currentActName.c_str(),
                 m_currentAct->duration, baked->frameCount, baked->frameTime, m_stateIndex);
    } else {
        LOG_WARN("[AnimPlayer] Bake produced no frames for '%s: %s'",
                 m_currentGroupName.c_str(), m_currentActName.c_str());
    }

    m_playing = true;
    m_time = 0.0f;
}

// ── Reset / Stop ───────────────────────────────────────────────────────────

void AnimationPlayer::Reset() {
    if (!m_skeleton) return;

    size_t jointCount = m_skeleton->joints.size();
    m_jointLocalPos.resize(jointCount);
    m_jointLocalRot.resize(jointCount);
    m_jointLocalScale.resize(jointCount);

    for (size_t i = 0; i < jointCount; ++i) {
        m_jointLocalPos[i] = m_skeleton->vectors4[i];
        m_jointLocalRot[i] = glm::vec4(
            (float)m_skeleton->vectors5[i].x,
            (float)m_skeleton->vectors5[i].y,
            (float)m_skeleton->vectors5[i].z,
            (float)m_skeleton->vectors5[i].w);
        m_jointLocalScale[i] = m_skeleton->vectors6[i];
    }

    m_time = 0.0f;

    if (jointCount > 2) {
        const auto& v5 = m_skeleton->vectors5[2];
        const auto& joint = m_skeleton->joints[2];
        LOG_INFO("[BindDiag] pelvis(j2) bindRot=(%d,%d,%d,%d) isQuat=%d name='%s'",
                 v5.x, v5.y, v5.z, v5.w, (int)joint.isQuaternion, joint.name.c_str());
    }
}

void AnimationPlayer::Stop() {
    m_playing = false;
    Reset();
}

// ── Time / Frame helpers ───────────────────────────────────────────────────

int AnimationPlayer::GetCurrentFrame() const {
    if (!m_currentBaked || m_currentBaked->frameTime <= 0.0f) return 0;
    int f = (int)std::round(m_time / m_currentBaked->frameTime);
    return std::clamp(f, 0, m_currentBaked->frameCount - 1);
}

void AnimationPlayer::SetTime(float t) {
    if (!m_currentAct) return;
    m_time = std::clamp(t, 0.0f, m_currentAct->duration);
    ApplyBakedAt(m_time);
}

void AnimationPlayer::SetFrame(int f) {
    if (!m_currentBaked) return;
    f = std::clamp(f, 0, m_currentBaked->frameCount - 1);
    SetTime((float)f * m_currentBaked->frameTime);
}

// ── ReturnStreamDataIndex (unchanged from JS port) ─────────────────────────

float AnimationPlayer::ReturnStreamDataIndex(const AnimSamplesManager& manager,
                                              const AnimSamplesManager& globalMgr,
                                              float animTime, float frameTime) const {
    float globalFrameCount = (float)(globalMgr.count + globalMgr.offset + globalMgr.datasCount3 - 1);
    float animFrame = animTime / frameTime;

    if ((animFrame + kEps) > globalFrameCount || (animFrame - kEps) < (float)manager.offset) {
        return -1.0f;
    }

    float streamSampleIdx = animFrame - (float)manager.offset;
    if (streamSampleIdx >= (float)(manager.count + manager.datasCount3)) {
        return -1.0f;
    } else if (streamSampleIdx > (float)(manager.count - 1)) {
        return (float)(manager.count - 1);
    } else if (streamSampleIdx < 0.0f) {
        return 0.0f;
    }
    return streamSampleIdx;
}

// ── HandleSkinningStream (delta accumulator, used by Bake walk only) ───────

bool AnimationPlayer::HandleSkinningStream(const AnimSubstream& stream,
                                            const AnimSamplesManager& globalMgr,
                                            float prevTime, float newTime,
                                            float frameTime,
                                            std::vector<glm::vec4>& jointData) {
    float newSamplePos = ReturnStreamDataIndex(stream.manager, globalMgr, newTime, frameTime);
    if (newSamplePos < 0.0f) return false;

    int newSampleIndex = (int)std::floor(newSamplePos);
    float newSampleOffset = newSamplePos - (float)newSampleIndex;

    bool changed = false;

    if (stream.isAdditive) {
        float newValueMultiplier = newSampleOffset;
        float prevSamplePos = ReturnStreamDataIndex(stream.manager, globalMgr, prevTime, frameTime);
        if (prevSamplePos >= 0.0f) {
            int prevSampleIndex = (int)std::floor(prevSamplePos);
            float prevSampleOffset = prevSamplePos - (float)prevSampleIndex;
            if (prevSampleIndex == newSampleIndex) {
                newValueMultiplier -= prevSampleOffset;
            } else {
                float prevValueMultiplier = 1.0f - prevSampleOffset;
                if (prevValueMultiplier > kEps) {
                    for (const auto& [iStream, samples] : stream.samples) {
                        if (iStream < 0) continue;
                        int jointId = iStream / 4;
                        int coord   = iStream % 4;
                        if (jointId < (int)jointData.size() && prevSampleIndex < (int)samples.size()) {
                            jointData[jointId][coord] += samples[prevSampleIndex] * prevValueMultiplier;
                            changed = true;
                        }
                    }
                }
            }
        }
        for (const auto& [iStream, samples] : stream.samples) {
            if (iStream < 0) continue;
            int jointId = iStream / 4;
            int coord   = iStream % 4;
            if (jointId < (int)jointData.size() && newSampleIndex < (int)samples.size()) {
                jointData[jointId][coord] += samples[newSampleIndex] * newValueMultiplier;
                changed = true;
            }
        }
    } else {
        int nextSampleIndex = newSampleIndex + 1;
        for (const auto& [iStream, samples] : stream.samples) {
            int jointId = iStream / 4;
            int coord   = iStream % 4;
            if (jointId >= (int)jointData.size()) continue;
            if (newSampleIndex >= (int)samples.size()) continue;

            float value;
            if (newSampleOffset < kEps) {
                value = samples[newSampleIndex];
            } else {
                float nextVal = (nextSampleIndex < (int)samples.size())
                              ? samples[nextSampleIndex]
                              : samples[newSampleIndex];
                value = samples[newSampleIndex] + (nextVal - samples[newSampleIndex]) * newSampleOffset;
            }
            jointData[jointId][coord] = value;
            changed = true;
        }
    }
    return changed;
}

// ── Bake: walk the act frame-by-frame and snapshot the pose ────────────────

BakedAnimation* AnimationPlayer::EnsureBaked() {
    if (!m_anim || !m_currentAct || !m_skeleton || m_stateIndex < 0) return nullptr;
    if (m_stateIndex >= (int)m_currentAct->stateDescrs.size()) return nullptr;
    if (m_anim->dataTypes[m_stateIndex].typeId != ANIM_DATATYPE_SKINNING) return nullptr;

    BakeKey key{m_anim, m_currentGroupIdx, m_currentActIdx};
    auto it = m_bakeCache.find(key);
    if (it != m_bakeCache.end()) {
        m_currentBaked = it->second.get();
        return m_currentBaked;
    }

    const auto& sd = m_currentAct->stateDescrs[m_stateIndex];
    float frameTime = sd.frameTime > 0.0f ? sd.frameTime : (1.0f / 30.0f);

    LOG_INFO("[BakeDiag] act='%s' states=%zu", m_currentActName.c_str(),
             sd.skinningStates.size());
    for (size_t si = 0; si < sd.skinningStates.size() && si < 4; ++si) {
        const auto& ss = sd.skinningStates[si];
        LOG_INFO("  state[%zu] rotRawCnt=%u rotAddSub=%zu rotRoughSub=%zu posRawCnt=%u posAddSub=%zu posRoughSub=%zu",
                 si, ss.rotationStream.manager.count,
                 ss.rotationSubStreamsAdd.size(), ss.rotationSubStreamsRough.size(),
                 ss.positionStream.manager.count,
                 ss.positionSubStreamsAdd.size(), ss.positionSubStreamsRough.size());
    }

    // Duration may be 0 for placeholder/idle acts — still bake a single frame
    // so the inspector can highlight the bind pose.
    int frameCount = 1;
    if (m_currentAct->duration > 0.0f) {
        frameCount = std::max(1, (int)std::ceil(m_currentAct->duration / frameTime) + 1);
    }

    auto baked = std::make_unique<BakedAnimation>();
    baked->frameCount = frameCount;
    baked->jointCount = (int)m_skeleton->joints.size();
    baked->frameTime  = frameTime;
    baked->duration   = m_currentAct->duration;
    baked->rotations.resize((size_t)frameCount * baked->jointCount);
    baked->positions.resize((size_t)frameCount * baked->jointCount);
    baked->scales.resize((size_t)frameCount * baked->jointCount);

    Reset();

    auto capture = [&](int frame) {
        size_t base = (size_t)frame * baked->jointCount;
        for (int j = 0; j < baked->jointCount; ++j) {
            baked->rotations[base + j] = m_jointLocalRot[j];
            baked->positions[base + j] = m_jointLocalPos[j];
            baked->scales[base + j]    = m_jointLocalScale[j];
        }
    };
    capture(0);

    // Advance the delta accumulator one frame at a time. HandleSkinningStream
    // already correctly handles single-frame steps; this loop just feeds it
    // a contiguous prev→next walk and snapshots each result.
    float prev = 0.0f;
    for (int f = 1; f < frameCount; ++f) {
        float curr = (float)f * frameTime;
        for (const auto& ss : sd.skinningStates) {
            if (ss.rotationStream.manager.count > 0) {
                HandleSkinningStream(ss.rotationStream, ss.rotationStream.manager,
                                     prev, curr, frameTime, m_jointLocalRot);
            } else {
                for (const auto& sub : ss.rotationSubStreamsAdd)
                    HandleSkinningStream(sub, ss.rotationStream.manager,
                                         prev, curr, frameTime, m_jointLocalRot);
                for (const auto& sub : ss.rotationSubStreamsRough)
                    HandleSkinningStream(sub, ss.rotationStream.manager,
                                         prev, curr, frameTime, m_jointLocalRot);
            }

            if (ss.positionStream.manager.count > 0) {
                HandleSkinningStream(ss.positionStream, ss.positionStream.manager,
                                     prev, curr, frameTime, m_jointLocalPos);
            } else {
                for (const auto& sub : ss.positionSubStreamsAdd)
                    HandleSkinningStream(sub, ss.positionStream.manager,
                                         prev, curr, frameTime, m_jointLocalPos);
                for (const auto& sub : ss.positionSubStreamsRough)
                    HandleSkinningStream(sub, ss.positionStream.manager,
                                         prev, curr, frameTime, m_jointLocalPos);
            }
        }
        capture(f);
        prev = curr;
    }

    LOG_INFO("[AnimBake] '%s' frames=%d joints=%d mem=%zu KB",
             m_currentActName.c_str(), frameCount, baked->jointCount,
             (baked->rotations.size() + baked->positions.size() + baked->scales.size())
                 * sizeof(glm::vec4) / 1024);

    BakedAnimation* raw = baked.get();
    m_bakeCache.emplace(key, std::move(baked));
    m_currentBaked = raw;
    return raw;
}

void AnimationPlayer::ApplyBakedAt(float time) {
    if (!m_currentBaked || m_currentBaked->empty()) return;
    const auto& b = *m_currentBaked;

    float fIdx = (b.frameTime > 0.0f) ? (time / b.frameTime) : 0.0f;
    if (fIdx < 0.0f) fIdx = 0.0f;
    float maxIdx = (float)(b.frameCount - 1);
    if (fIdx > maxIdx) fIdx = maxIdx;

    int f0 = (int)std::floor(fIdx);
    int f1 = std::min(f0 + 1, b.frameCount - 1);
    float t = fIdx - (float)f0;

    size_t base0 = (size_t)f0 * b.jointCount;
    size_t base1 = (size_t)f1 * b.jointCount;

    m_jointLocalPos.resize(b.jointCount);
    m_jointLocalRot.resize(b.jointCount);
    m_jointLocalScale.resize(b.jointCount);

    if (t < kEps) {
        for (int j = 0; j < b.jointCount; ++j) {
            m_jointLocalRot[j]   = b.rotations[base0 + j];
            m_jointLocalPos[j]   = b.positions[base0 + j];
            m_jointLocalScale[j] = b.scales[base0 + j];
        }
    } else {
        const auto& joints = m_skeleton->joints;
        for (int j = 0; j < b.jointCount; ++j) {
            const glm::vec4& r0 = b.rotations[base0 + j];
            const glm::vec4& r1 = b.rotations[base1 + j];

            bool isQuatJoint = (j < (int)joints.size()) && joints[j].isQuaternion;
            if (isQuatJoint) {
                // Quaternion path: convert Q14 → normalized quat → slerp → back to Q14 vec4.
                auto toQuat = [](const glm::vec4& v) {
                    glm::quat q(v.w * kQuatToFloat, v.x * kQuatToFloat,
                                v.y * kQuatToFloat, v.z * kQuatToFloat);
                    float L = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
                    if (L > 0.0001f) { q.x/=L; q.y/=L; q.z/=L; q.w/=L; }
                    else             { q.x=0;  q.y=0;  q.z=0;  q.w=1; }
                    return q;
                };
                glm::quat q0 = toQuat(r0);
                glm::quat q1 = toQuat(r1);
                if (q0.w*q1.w + q0.x*q1.x + q0.y*q1.y + q0.z*q1.z < 0.0f) {
                    q1 = glm::quat(-q1.w, -q1.x, -q1.y, -q1.z);
                }
                glm::quat qi = glm::slerp(q0, q1, t);
                m_jointLocalRot[j] = glm::vec4(qi.x, qi.y, qi.z, qi.w) * (float)(1 << 14);
            } else {
                m_jointLocalRot[j] = glm::mix(r0, r1, t);
            }
            m_jointLocalPos[j]   = glm::mix(b.positions[base0 + j], b.positions[base1 + j], t);
            m_jointLocalScale[j] = glm::mix(b.scales[base0 + j],    b.scales[base1 + j],    t);
        }
    }
}

// ── Update ─────────────────────────────────────────────────────────────────

bool AnimationPlayer::Update(float dt) {
    if (!m_playing || !m_currentAct || !m_currentBaked) return false;
    if (m_currentBaked->empty()) return false;

    float dur = m_currentAct->duration;
    if (dur <= 0.0f) {
        ApplyBakedAt(0.0f);
        m_time = 0.0f;
        return true;
    }

    float step = dt * m_speed * (float)m_pingPongDir;
    float newTime = m_time + step;

    switch (m_loopMode) {
        case LoopMode::None:
            if (newTime >= dur)      { newTime = dur;  m_playing = false; }
            else if (newTime < 0.0f) { newTime = 0.0f; m_playing = false; }
            break;
        case LoopMode::Loop:
            newTime = std::fmod(newTime, dur);
            if (newTime < 0.0f) newTime += dur;
            break;
        case LoopMode::PingPong:
            // Bounce on either end; clamp the overshoot and flip direction.
            if (newTime >= dur)      { newTime = dur  - (newTime - dur);  m_pingPongDir = -1; }
            else if (newTime < 0.0f) { newTime = -newTime;                 m_pingPongDir = +1; }
            if (newTime < 0.0f) newTime = 0.0f;
            if (newTime > dur)  newTime = dur;
            break;
    }

    ApplyBakedAt(newTime);
    m_time = newTime;
    return true;
}

// ── ComputeJointMatrices ───────────────────────────────────────────────────

std::vector<glm::mat4> AnimationPlayer::ComputeJointMatrices() const {
    if (!m_skeleton) return {};

    size_t jointCount = m_skeleton->joints.size();
    std::vector<glm::mat4> localMats(jointCount, glm::mat4(1.0f));
    std::vector<glm::mat4> globalMats(jointCount, glm::mat4(1.0f));

    for (size_t i = 0; i < jointCount; ++i) {
        const auto& joint = m_skeleton->joints[i];
        const auto& rot = m_jointLocalRot[i];
        const auto& pos = m_jointLocalPos[i];
        const auto& scl = m_jointLocalScale[i];

        glm::quat localQ;
        if (joint.isQuaternion) {
            float qx = rot.x * kQuatToFloat;
            float qy = rot.y * kQuatToFloat;
            float qz = rot.z * kQuatToFloat;
            float qw = rot.w * kQuatToFloat;
            float qlen = std::sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
            if (qlen > 0.0001f) { qx/=qlen; qy/=qlen; qz/=qlen; qw/=qlen; }
            else                { qx=0; qy=0; qz=0; qw=1; }
            localQ = glm::quat(qw, qx, qy, qz);
        } else {
            const float halfToRad = (0.5f * glm::pi<float>()) / 180.0f;
            float ex = rot.x * kQuatToFloat * 360.0f * halfToRad;
            float ey = rot.y * kQuatToFloat * 360.0f * halfToRad;
            float ez = rot.z * kQuatToFloat * 360.0f * halfToRad;
            float sx = std::sin(ex), cx = std::cos(ex);
            float sy = std::sin(ey), cy = std::cos(ey);
            float sz = std::sin(ez), cz = std::cos(ez);
            float qx = sx*cy*cz - cx*sy*sz;
            float qy = cx*sy*cz + sx*cy*sz;
            float qz = cx*cy*sz - sx*sy*cz;
            float qw = cx*cy*cz + sx*sy*sz;
            float qlen = std::sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
            if (qlen > 0.0001f) { qx/=qlen; qy/=qlen; qz/=qlen; qw/=qlen; }
            else                { qx=0; qy=0; qz=0; qw=1; }
            localQ = glm::quat(qw, qx, qy, qz);
        }

        glm::mat4 local = glm::mat4_cast(localQ);
        local[0] *= (scl.x != 0.0f ? scl.x : 1.0f);
        local[1] *= (scl.y != 0.0f ? scl.y : 1.0f);
        local[2] *= (scl.z != 0.0f ? scl.z : 1.0f);
        local[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);

        localMats[i] = local;

        if (joint.parent >= 0 && joint.parent < (int)jointCount) {
            globalMats[i] = globalMats[joint.parent] * local;
        } else {
            globalMats[i] = local;
        }
    }

    std::vector<glm::mat4> result(jointCount, glm::mat4(1.0f));
    for (size_t i = 0; i < jointCount; ++i) {
        const auto& joint = m_skeleton->joints[i];
        if (joint.isSkinned && joint.invId >= 0 && joint.invId < (int)m_skeleton->matrixes3.size()) {
            result[i] = globalMats[i] * m_skeleton->matrixes3[joint.invId];
        } else {
            result[i] = globalMats[i];
        }
    }
    return result;
}

} // namespace GOW
