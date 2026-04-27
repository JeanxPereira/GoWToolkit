// AnimationPlayer — runtime skeletal animation evaluator
// Port of AnimationObjectSkelet from Animation.js in the Go browser.

#include "AnimationPlayer.h"
#include "core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <algorithm>

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

    // Find skinning data type index
    m_stateIndex = anim->FindSkinningTypeIndex();

    Reset();
    m_playing = true;

    LOG_INFO("[AnimPlayer] Playing '%s: %s' (duration=%.3fs, stateIdx=%d)",
             m_currentGroupName.c_str(), m_currentActName.c_str(),
             m_currentAct->duration, m_stateIndex);

    // Diagnostic: log the stream manager values for this act
    if (m_stateIndex >= 0 && m_stateIndex < (int)m_currentAct->stateDescrs.size()) {
        const auto& sd = m_currentAct->stateDescrs[m_stateIndex];
        size_t rotAdd = 0, rotRough = 0, posAdd = 0, posRough = 0;
        size_t totalSamples = 0;

        for (const auto& ss : sd.skinningStates) {
            rotAdd += ss.rotationSubStreamsAdd.size();
            rotRough += ss.rotationSubStreamsRough.size();
            posAdd += ss.positionSubStreamsAdd.size();
            posRough += ss.positionSubStreamsRough.size();

            totalSamples += ss.rotationStream.samples.size() + ss.positionStream.samples.size();
            for (const auto& s : ss.rotationSubStreamsAdd) totalSamples += s.samples.size();
            for (const auto& s : ss.rotationSubStreamsRough) totalSamples += s.samples.size();
            for (const auto& s : ss.positionSubStreamsAdd) totalSamples += s.samples.size();
            for (const auto& s : ss.positionSubStreamsRough) totalSamples += s.samples.size();
        }

        LOG_INFO("[AnimDiag] skinningStates=%zu | rotSubAdd=%zu rotSubRough=%zu posSubAdd=%zu posSubRough=%zu",
                 sd.skinningStates.size(), rotAdd, rotRough, posAdd, posRough);
        LOG_INFO("[AnimDiag] Total sample entries: %zu", totalSamples);
    }
}

// ── Reset to idle pose ─────────────────────────────────────────────────────

void AnimationPlayer::Reset() {
    if (!m_skeleton) return;

    size_t jointCount = m_skeleton->joints.size();

    m_jointLocalPos.resize(jointCount);
    m_jointLocalRot.resize(jointCount);
    m_jointLocalScale.resize(jointCount);

    for (size_t i = 0; i < jointCount; ++i) {
        m_jointLocalPos[i] = m_skeleton->vectors4[i];

        // Convert Q.14 ivec4 → float vec4 (matching JS: obj.Vectors5[i].slice())
        m_jointLocalRot[i] = glm::vec4(
            (float)m_skeleton->vectors5[i].x,
            (float)m_skeleton->vectors5[i].y,
            (float)m_skeleton->vectors5[i].z,
            (float)m_skeleton->vectors5[i].w
        );

        m_jointLocalScale[i] = m_skeleton->vectors6[i];
    }

    m_time = 0.0f;
}

// ── Stop ───────────────────────────────────────────────────────────────────

void AnimationPlayer::Stop() {
    m_playing = false;
    Reset();
}

// ── SetTime (seek) ─────────────────────────────────────────────────────────

void AnimationPlayer::SetTime(float t) {
    if (!m_currentAct) return;
    // Reset to idle then play forward to target time
    Reset();
    float prevTime = 0.0f;
    m_time = std::clamp(t, 0.0f, m_currentAct->duration);
    // Re-evaluate from 0 to target
    if (m_stateIndex >= 0 && m_stateIndex < (int)m_currentAct->stateDescrs.size()) {
        const auto& sd = m_currentAct->stateDescrs[m_stateIndex];
        if (m_anim->dataTypes[m_stateIndex].typeId == ANIM_DATATYPE_SKINNING) {
            for (const auto& ss : sd.skinningStates) {
                // Process rotation streams
                if (ss.rotationStream.manager.count > 0) {
                    HandleSkinningStream(ss.rotationStream, ss.rotationStream.manager,
                                       prevTime, m_time, sd.frameTime, m_jointLocalRot);
                } else {
                    for (const auto& sub : ss.rotationSubStreamsAdd) {
                        HandleSkinningStream(sub, ss.rotationStream.manager,
                                           prevTime, m_time, sd.frameTime, m_jointLocalRot);
                    }
                    for (const auto& sub : ss.rotationSubStreamsRough) {
                        HandleSkinningStream(sub, ss.rotationStream.manager,
                                           prevTime, m_time, sd.frameTime, m_jointLocalRot);
                    }
                }
                // Process position streams
                if (ss.positionStream.manager.count > 0) {
                    HandleSkinningStream(ss.positionStream, ss.positionStream.manager,
                                       prevTime, m_time, sd.frameTime, m_jointLocalPos);
                } else {
                    for (const auto& sub : ss.positionSubStreamsAdd) {
                        HandleSkinningStream(sub, ss.positionStream.manager,
                                           prevTime, m_time, sd.frameTime, m_jointLocalPos);
                    }
                    for (const auto& sub : ss.positionSubStreamsRough) {
                        HandleSkinningStream(sub, ss.positionStream.manager,
                                           prevTime, m_time, sd.frameTime, m_jointLocalPos);
                    }
                }
            }
        }
    }
}

// ── ReturnStreamDataIndex ──────────────────────────────────────────────────
// Port of AnimationObjectSkelet.returnStreamDataIndex from Animation.js

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

// ── HandleSkinningStream ───────────────────────────────────────────────────
// Port of AnimationObjectSkelet.handleSkinningStream from Animation.js

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
        // Additive change
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
                    // Apply remaining prev sample
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
        // Exact (raw) change
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

// ── Update ─────────────────────────────────────────────────────────────────

bool AnimationPlayer::Update(float dt) {
    if (!m_playing || !m_currentAct || m_stateIndex < 0) return false;
    if (m_stateIndex >= (int)m_currentAct->stateDescrs.size()) return false;

    float newTime = m_time + dt;
    const auto& sd = m_currentAct->stateDescrs[m_stateIndex];

    if (m_anim->dataTypes[m_stateIndex].typeId != ANIM_DATATYPE_SKINNING) return false;

    bool changed = false;

    // Diagnostic: log once per play
    static int updateDiagCounter = 0;
    bool logThisFrame = (updateDiagCounter++ % 120 == 0);

    for (const auto& ss : sd.skinningStates) {
        // Process rotation streams
        if (ss.rotationStream.manager.count > 0) {
            if (HandleSkinningStream(ss.rotationStream, ss.rotationStream.manager,
                                    m_time, newTime, sd.frameTime, m_jointLocalRot))
                changed = true;
        } else {
            for (const auto& sub : ss.rotationSubStreamsAdd) {
                if (HandleSkinningStream(sub, ss.rotationStream.manager,
                                        m_time, newTime, sd.frameTime, m_jointLocalRot))
                    changed = true;
            }
            for (const auto& sub : ss.rotationSubStreamsRough) {
                if (HandleSkinningStream(sub, ss.rotationStream.manager,
                                        m_time, newTime, sd.frameTime, m_jointLocalRot))
                    changed = true;
            }
        }

        // Process position streams
        if (ss.positionStream.manager.count > 0) {
            if (HandleSkinningStream(ss.positionStream, ss.positionStream.manager,
                                    m_time, newTime, sd.frameTime, m_jointLocalPos))
                changed = true;
        } else {
            for (const auto& sub : ss.positionSubStreamsAdd) {
                if (HandleSkinningStream(sub, ss.positionStream.manager,
                                        m_time, newTime, sd.frameTime, m_jointLocalPos))
                    changed = true;
            }
            for (const auto& sub : ss.positionSubStreamsRough) {
                if (HandleSkinningStream(sub, ss.positionStream.manager,
                                        m_time, newTime, sd.frameTime, m_jointLocalPos))
                    changed = true;
            }
        }
    }

    if (logThisFrame) {
        LOG_INFO("[AnimUpd] time=%.3f->%.3f changed=%d rot[0]=(%.1f,%.1f,%.1f,%.1f)",
                 m_time, newTime, changed,
                 m_jointLocalRot.empty() ? 0.f : m_jointLocalRot[0].x,
                 m_jointLocalRot.empty() ? 0.f : m_jointLocalRot[0].y,
                 m_jointLocalRot.empty() ? 0.f : m_jointLocalRot[0].z,
                 m_jointLocalRot.empty() ? 0.f : m_jointLocalRot[0].w);
    }

    m_time = newTime;

    // Loop or stop at end
    if (m_time >= m_currentAct->duration) {
        if (m_loop) {
            Reset();
        }
    }

    return changed;
}

// ── ComputeJointMatrices ───────────────────────────────────────────────────
// Port of AnimationObjectSkelet.recalcMatrices from Animation.js

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
            // Quaternion Q.14: {x,y,z,w} = v5[0..3] * Q14, then normalize
            float qx = float(rot.x) * kQuatToFloat;
            float qy = float(rot.y) * kQuatToFloat;
            float qz = float(rot.z) * kQuatToFloat;
            float qw = float(rot.w) * kQuatToFloat;
            float qlen = std::sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
            if (qlen > 0.0001f) { qx/=qlen; qy/=qlen; qz/=qlen; qw/=qlen; }
            else                { qx=0; qy=0; qz=0; qw=1; }
            localQ = glm::quat(qw, qx, qy, qz);
        } else {
            // Intrinsic Euler ZYX in degrees
            const float halfToRad = (0.5f * glm::pi<float>()) / 180.0f;
            float ex = float(rot.x) * kQuatToFloat * 360.0f * halfToRad;
            float ey = float(rot.y) * kQuatToFloat * 360.0f * halfToRad;
            float ez = float(rot.z) * kQuatToFloat * 360.0f * halfToRad;
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
        // Apply scale (prevent 0 scale collapse)
        local[0] *= (scl.x != 0.0f ? scl.x : 1.0f);
        local[1] *= (scl.y != 0.0f ? scl.y : 1.0f);
        local[2] *= (scl.z != 0.0f ? scl.z : 1.0f);
        // Apply translation
        local[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);

        localMats[i] = local;

        // Compute global matrix
        if (joint.parent >= 0 && joint.parent < (int)jointCount) {
            globalMats[i] = globalMats[joint.parent] * local;
        } else {
            globalMats[i] = local;
        }
    }

    // Compute final skinning matrices (global * inverse bind pose)
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
