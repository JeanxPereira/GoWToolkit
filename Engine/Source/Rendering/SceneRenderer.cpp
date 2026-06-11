#include "SceneRenderer.h"
#include "ShaderManager.h"
#include "Core/AppConfig.h"
#include "Core/Logger.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace Onyx {

// â”€â”€ Texture upload â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

GLuint SceneRenderer::UploadTexture(const TextureData& tex) {
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 tex.width, tex.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, tex.pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

// â”€â”€ Build from SceneData â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SceneRenderer::Build(const SceneData& scene) {
    Clear();

    m_skeleton = scene.skeleton;
    m_animData = scene.animations;
    // GOW2 models face -Z; GOWR is already in screen-correct space.
    m_instanceTransform = scene.flipZ
        ? glm::scale(scene.instanceTransform, glm::vec3(1.0f, 1.0f, -1.0f))
        : scene.instanceTransform;

    // Upload textures from SceneData (indexed by materialId, then layer)
    std::vector<std::vector<GLuint>> textureIds(scene.textures.size());
    for (size_t i = 0; i < scene.textures.size(); ++i) {
        textureIds[i].reserve(scene.textures[i].size());
        for (size_t j = 0; j < scene.textures[i].size(); ++j) {
            GLuint texId = 0;
            if (scene.textures[i][j] && scene.textures[i][j]->IsValid()) {
                texId = UploadTexture(*scene.textures[i][j]);
                m_ownedTextures.push_back(texId);
            }
            textureIds[i].push_back(texId);
        }
    }

    // Build render batches from mesh parts
    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());

    m_batches.reserve(scene.meshParts.size());
    for (const auto& part : scene.meshParts) {
        if (part.vertices.empty() || part.indices.empty()) continue;

        RenderBatch batch;
        batch.name = part.name;

        // Upload geometry
        auto mesh = std::make_shared<GpuMesh>();
        mesh->Upload(part.vertices, part.indices);
        batch.gpuMesh = mesh;
        batch.textureLayer = part.textureLayer;
        batch.jointMap = part.jointMap;
        batch.hasSkeleton = scene.HasSkeleton();
        batch.isSky = part.isSky;
        batch.meshHash = part.meshHash;
        batch.vertexCount = (int)part.vertices.size();
        batch.triangleCount = (int)part.indices.size() / 3;

        // Bounds â€” computed from transformed positions so FocusOn targets
        // rendered space and the camera's projection slab covers every vertex
        // (including the sky dome). FocusOn separately handles centering.
        for (const auto& v : part.vertices) {
            glm::vec3 tp = glm::vec3(m_instanceTransform * glm::vec4(v.position, 1.0f));
            boundsMin = glm::min(boundsMin, tp);
            boundsMax = glm::max(boundsMax, tp);
        }

        // Resolve material
        if (part.materialId < scene.materials.size()) {
            const auto& mat = scene.materials[part.materialId];
            std::memcpy(batch.materialColor, mat.baseColor, sizeof(float) * 4);

            if (part.textureLayer < mat.layers.size()) {
                const auto& layerConf = mat.layers[part.textureLayer];
                std::memcpy(batch.layerColor, layerConf.blendColor, sizeof(float) * 4);
                batch.blendMode = layerConf.blendMode;
                batch.uvOffset[0] = layerConf.uvOffset[0];
                batch.uvOffset[1] = layerConf.uvOffset[1];
            } else if (!mat.layers.empty()) {
                const auto& layer0 = mat.layers[0];
                std::memcpy(batch.layerColor, layer0.blendColor, sizeof(float) * 4);
                batch.blendMode = layer0.blendMode;
                batch.uvOffset[0] = layer0.uvOffset[0];
                batch.uvOffset[1] = layer0.uvOffset[1];
            }
        }

        // Resolve diffuse texture using textureLayer
        if (part.materialId < textureIds.size()) {
            if (part.textureLayer < textureIds[part.materialId].size() && textureIds[part.materialId][part.textureLayer] != 0) {
                batch.texture0 = textureIds[part.materialId][part.textureLayer];
                batch.hasTexture = true;
            }
        }

        m_batches.push_back(std::move(batch));

        // DIAGNOSTIC â€” log first batch's jointMap and first 5 vertex boneIndices/positions
        if (m_batches.size() == 1) {
            const auto& jm = m_batches[0].jointMap;
            std::string jmStr = "jointMap[0.." + std::to_string(std::min<size_t>(jm.size(), 4)) + "]=";
            for (size_t k = 0; k < std::min<size_t>(jm.size(), 4); ++k)
                jmStr += std::to_string(jm[k]) + " ";
            LOG_INFO("[JDiag] batch0: %s (cnt=%zu)", jmStr.c_str(), jm.size());
            size_t nv = std::min<size_t>(part.vertices.size(), 5);
            for (size_t k = 0; k < nv; ++k) {
                const auto& v = part.vertices[k];
                LOG_INFO("  v[%zu] pos=(%.3f %.3f %.3f) bones=(%u %u) weights=(%.3f %.3f)",
                         k, v.position.x, v.position.y, v.position.z,
                         v.boneIndices.x, v.boneIndices.y,
                         v.boneWeights.x, v.boneWeights.y);
            }
        }
    }

    m_bounds.min = boundsMin;
    m_bounds.max = boundsMax;

    LOG_INFO("[SceneRenderer] Bounds: min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f) center=(%.2f, %.2f, %.2f) radius=%.2f",
             boundsMin.x, boundsMin.y, boundsMin.z,
             boundsMax.x, boundsMax.y, boundsMax.z,
             m_bounds.Center().x, m_bounds.Center().y, m_bounds.Center().z,
             m_bounds.Radius());

    m_hasSky = scene.isSky;

    if (m_skeleton) {
        ComputeJointPalette();
    }

    // Sort into opaque vs translucent batches (additive or layered)
    for (auto& batch : m_batches) {
        if (batch.isSky) {
            m_skyBatches.push_back(&batch);
        } else if (batch.blendMode != BlendMode::Normal || batch.textureLayer > 0) {
            m_additiveBatches.push_back(&batch);
        } else {
            m_opaqueBatches.push_back(&batch);
        }
    }

    LOG_INFO("[SceneRenderer] Built %zu batches (%zu opaque, %zu additive, %zu sky), skeleton=%s",
             m_batches.size(), m_opaqueBatches.size(), m_additiveBatches.size(), m_skyBatches.size(),
             m_skeleton ? "yes" : "no");
}

// â”€â”€ Build from simple MeshData â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SceneRenderer::BuildFromMeshData(const MeshData& data,
                                       const std::vector<std::unique_ptr<TextureData>>& textures) {
    Clear();

    // Flip Z axis: GOW2 models face -Z, we want them facing the camera
    m_instanceTransform = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, -1.0f));

    // Upload textures
    std::vector<GLuint> textureIds(textures.size(), 0);
    for (size_t i = 0; i < textures.size(); ++i) {
        if (textures[i] && textures[i]->IsValid()) {
            GLuint texId = UploadTexture(*textures[i]);
            textureIds[i] = texId;
            m_ownedTextures.push_back(texId);
        }
    }

    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());

    m_batches.reserve(data.parts.size());
    for (const auto& part : data.parts) {
        if (part.vertices.empty() || part.indices.empty()) continue;

        RenderBatch batch;
        batch.name = part.name;

        auto mesh = std::make_shared<GpuMesh>();
        mesh->Upload(part.vertices, part.indices);
        batch.gpuMesh = mesh;
        batch.meshHash = part.meshHash;
        batch.vertexCount = (int)part.vertices.size();
        batch.triangleCount = (int)part.indices.size() / 3;

        for (const auto& v : part.vertices) {
            glm::vec3 tp = glm::vec3(m_instanceTransform * glm::vec4(v.position, 1.0f));
            boundsMin = glm::min(boundsMin, tp);
            boundsMax = glm::max(boundsMax, tp);
        }

        // Resolve texture by material ID
        if (part.materialId >= 0 && part.materialId < (int)textureIds.size()
            && textureIds[part.materialId] != 0) {
            batch.texture0 = textureIds[part.materialId];
            batch.hasTexture = true;
        }

        m_batches.push_back(std::move(batch));
    }

    m_bounds.min = boundsMin;
    m_bounds.max = boundsMax;

    // All batches opaque for simple meshes
    for (auto& batch : m_batches) {
        m_opaqueBatches.push_back(&batch);
    }

    LOG_INFO("[SceneRenderer] Built %zu batches from MeshData. Bounds min=(%.3f,%.3f,%.3f) max=(%.3f,%.3f,%.3f) center=(%.3f,%.3f,%.3f) radius=%.3f",
             m_batches.size(),
             boundsMin.x, boundsMin.y, boundsMin.z,
             boundsMax.x, boundsMax.y, boundsMax.z,
             m_bounds.Center().x, m_bounds.Center().y, m_bounds.Center().z,
             m_bounds.Radius());
}

// â”€â”€ Joint palette â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Compute two arrays in parallel:
//   m_jointPalette[i]  = skinning matrix  = worldRestPose[i] * bindToJointMat[i]
//   m_jointWorldPos[i] = world-space joint origin (for skeleton debug draw)
//
// Port of GLTF export (obj/export_gltf.go):
//   - Each GLTF joint node has Translation=Vectors4, Rotation=Vectors5, Scale=Vectors6.
//   - InverseBindMatrix = BindToJointMat (= Matrixes3[invId]).
//   - GLTF skinning: jointMatrix = globalTransform(node) * inverseBindMatrix.
//
// We reconstruct globalTransform by walking the parent chain and multiplying
// local TRS matrices (built from Vectors4/5/6) â€” NOT from Matrixes1.
// Matrixes1 is parentToJoint in the PS2 VU microcode sense and is NOT the same
// as the GLTF-equivalent TRS local matrix; using it caused all verts to collapse.

static glm::mat4 BuildLocalTRS(const Onyx::ObjectData& obj, int i) {
    // Port exato de obj/export_gltf.go + obj.go GetQuaterionLocalRotationForJoint:
    //
    // GOW2 (obj_gow2.go): IsQuaterion NUNCA Ã© setado â†’ sempre false.
    // GOW1 (obj.go):      IsQuaterion = flags & 0x8000.
    //
    // Quando false (Euler):
    //   euler_deg = {v5[0], v5[1], v5[2]} * (1/16384 * 360)  [graus]
    //   rotation  = EulerToQuat(euler_deg)  â€” ZYX intrÃ­nseco
    //
    // Quando true (Quaternion):
    //   quat = {v5[0], v5[1], v5[2], v5[3]} * (1/16384)  [normalizado]

    const auto& v4 = obj.vectors4[i];  // local translation
    const auto& v5 = obj.vectors5[i];  // rotation (Q.14 fixed-point)
    const auto& v6 = obj.vectors6[i];  // local scale

    const float Q14 = 1.0f / (1 << 14);

    // â”€â”€ Scale â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    glm::mat4 S = glm::scale(glm::mat4(1.0f),
                             glm::vec3(v6.x != 0.0f ? v6.x : 1.0f,
                                       v6.y != 0.0f ? v6.y : 1.0f,
                                       v6.z != 0.0f ? v6.z : 1.0f));

    // â”€â”€ Rotation â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    glm::quat rot;
    const bool& isQuat = obj.joints[i].isQuaternion;

    if (isQuat) {
        // Quaternion Q.14: {x,y,z,w} = v5[0..3] * Q14, depois normaliza
        float qx = float(v5.x) * Q14;
        float qy = float(v5.y) * Q14;
        float qz = float(v5.z) * Q14;
        float qw = float(v5.w) * Q14;
        float qlen = std::sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
        if (qlen > 0.0001f) { qx/=qlen; qy/=qlen; qz/=qlen; qw/=qlen; }
        else                 { qx=0; qy=0; qz=0; qw=1; }
        rot = glm::quat(qw, qx, qy, qz);
    } else {
        // Euler ZYX intrÃ­nseco em graus (port de utils/math.go EulerToQuat)
        // euler_deg = {v5[0],v5[1],v5[2]} * Q14 * 360
        const float halfToRad = (0.5f * glm::pi<float>()) / 180.0f;
        float ex = float(v5.x) * Q14 * 360.0f * halfToRad;  // Ã¢ngulo X / 2
        float ey = float(v5.y) * Q14 * 360.0f * halfToRad;  // Ã¢ngulo Y / 2
        float ez = float(v5.z) * Q14 * 360.0f * halfToRad;  // Ã¢ngulo Z / 2
        float sx = std::sin(ex), cx = std::cos(ex);
        float sy = std::sin(ey), cy = std::cos(ey);
        float sz = std::sin(ez), cz = std::cos(ez);
        // ZYX: qx = sx*cy*cz - cx*sy*sz
        //      qy = cx*sy*cz + sx*cy*sz
        //      qz = cx*cy*sz - sx*sy*cz
        //      qw = cx*cy*cz + sx*sy*sz
        float qx = sx*cy*cz - cx*sy*sz;
        float qy = cx*sy*cz + sx*cy*sz;
        float qz = cx*cy*sz - sx*sy*cz;
        float qw = cx*cy*cz + sx*sy*sz;
        float qlen = std::sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
        if (qlen > 0.0001f) { qx/=qlen; qy/=qlen; qz/=qlen; qw/=qlen; }
        else                 { qx=0; qy=0; qz=0; qw=1; }
        rot = glm::quat(qw, qx, qy, qz);
    }

    glm::mat4 R = glm::mat4_cast(rot);

    // â”€â”€ Translation â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(v4.x, v4.y, v4.z));

    return T * R * S;
}

// â”€â”€ Animation API â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SceneRenderer::SetAnimation(int groupIdx, int actIdx) {
    if (!m_animData || !m_skeleton) return;
    if (!m_animPlayer) {
        m_animPlayer = std::make_unique<AnimationPlayer>();
    }
    m_animPlayer->SetAnimation(m_animData.get(), groupIdx, actIdx, m_skeleton.get());
}

void SceneRenderer::StopAnimation() {
    if (m_animPlayer) {
        m_animPlayer->Stop();
        // Recompute idle joint palette
        ComputeJointPalette();
    }
}

bool SceneRenderer::UpdateAnimation(float dt) {
    if (!m_animPlayer) return false;

    bool changed = false;
    if (m_animPlayer->IsPlaying()) {
        changed = m_animPlayer->Update(dt);
    } else {
        // Detect external scrubbing (SetTime/SetFrame called from UI while
        // paused): the player advanced its pose but Update() didn't run, so
        // refresh the palette manually when its time differs from ours.
        float t = m_animPlayer->GetTime();
        if (t != m_lastAppliedAnimTime) {
            changed = true;
        }
    }
    if (changed) {
        // Diagnostic: save pelvis before
        glm::vec4 pelvisBefore(0);
        if (m_jointPalette.size() > 2) pelvisBefore = m_jointPalette[2][3];

        // Override joint palette with animated pose
        auto animatedMats = m_animPlayer->ComputeJointMatrices();
        if (!animatedMats.empty()) {
            m_jointPalette = std::move(animatedMats);
            // Update debug world positions from animated palette
            m_jointWorldPos.resize(m_jointPalette.size());
            for (size_t i = 0; i < m_jointPalette.size(); ++i) {
                m_jointWorldPos[i] = glm::vec3(m_jointPalette[i][3]);
            }
        }

        // Diagnostic: compare
        static int animDiagCnt = 0;
        if (animDiagCnt++ % 120 == 0 && m_jointPalette.size() > 2) {
            glm::vec4 pelvisAfter = m_jointPalette[2][3];
            LOG_INFO("[PalDiag] pelvis palette[2].trans: before=(%.2f,%.2f,%.2f) after=(%.2f,%.2f,%.2f)",
                     pelvisBefore.x, pelvisBefore.y, pelvisBefore.z,
                     pelvisAfter.x, pelvisAfter.y, pelvisAfter.z);
            // Also print rotation of pelvis in animated local data
            auto localRot = m_animPlayer->GetJointRotations();
            if (localRot.size() > 2) {
                LOG_INFO("[PalDiag] pelvis localRot[2]=(%.1f,%.1f,%.1f,%.1f)",
                         localRot[2].x, localRot[2].y, localRot[2].z, localRot[2].w);
            }
        }
        m_lastAppliedAnimTime = m_animPlayer->GetTime();
    }
    return changed;
}

// â”€â”€ Joint palette â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SceneRenderer::ComputeJointPalette() {
    if (!m_skeleton) return;

    const auto& sk = *m_skeleton;
    size_t N = sk.joints.size();

    // Build the idle palette using the same TRS chain (Vectors4/5/6) that
    // AnimationPlayer::ComputeJointMatrices uses on every frame. JS Animation.js
    // (AnimationObjectSkelet.recalcMatrices) does this unconditionally:
    //
    //     local = mat4.fromRotationTranslationScale(localQ, V4[i], V6[i]);
    //     treeNode.joints[i].setLocalMatrixWithoutUpdate(local);   // V5 â†’ quat
    //
    // Previously we used Matrixes1 chain (parentToJoint stored in the file)
    // for idle and TRS for animated. Those produce different world poses for
    // joints whose Matrixes1 encodes a fixed offset that V4 does not â€” e.g.
    // PalDiag log shows idle pelvis trans (0,0,0) vs animated (-5.8,14.4,-30).
    // The first animated frame snapped every skinned vertex by that delta.
    // Matrixes3 (the inverse bind pose) was computed against the TRS pose,
    // so the TRS chain is the canonical one and Matrixes1 is the outlier.

    m_jointPalette.resize(N);
    m_jointWorldPos.resize(N);

    std::vector<glm::mat4> globalMats(N, glm::mat4(1.0f));
    for (size_t i = 0; i < N; ++i) {
        const auto& j = sk.joints[i];
        glm::mat4 local = BuildLocalTRS(sk, (int)i);
        if (j.parent >= 0 && j.parent < (int)N) {
            globalMats[i] = globalMats[j.parent] * local;
        } else {
            globalMats[i] = local;
        }
        m_jointPalette[i]  = globalMats[i] * j.bindToJointMat;
        m_jointWorldPos[i] = glm::vec3(globalMats[i][3]);
    }

    // DIAGNOSTIC â€” log first 3 joints once per scene load
    if (!m_diagDone) { m_diagDone = true;
        for (size_t di = 0; di < std::min<size_t>(N, 3); ++di) {
            const auto& dj = sk.joints[di];
            const auto& W = dj.renderMat; const auto& P = m_jointPalette[di];
            const auto& B = dj.bindToJointMat;
            LOG_INFO("[JDiag] j[%zu]='%s' isQuat=%d parent=%d skinned=%d invId=%d",
                     di,dj.name.c_str(),(int)dj.isQuaternion,(int)dj.parent,(int)dj.isSkinned,(int)dj.invId);
            LOG_INFO("  V4=(%.3f %.3f %.3f)  V5=(%d %d %d %d)  V6=(%.3f %.3f %.3f)",
                     sk.vectors4[di].x,sk.vectors4[di].y,sk.vectors4[di].z,
                     sk.vectors5[di].x,sk.vectors5[di].y,sk.vectors5[di].z,sk.vectors5[di].w,
                     sk.vectors6[di].x,sk.vectors6[di].y,sk.vectors6[di].z);
            LOG_INFO("  renderMat.trans=(%.4f %.4f %.4f)", W[3][0], W[3][1], W[3][2]);
            LOG_INFO("  bindMat.trans =(%.4f %.4f %.4f)", B[3][0], B[3][1], B[3][2]);
            LOG_INFO("  palette.trans =(%.4f %.4f %.4f) <-- skinning offset",
                     P[3][0], P[3][1], P[3][2]);
        }
    }
}

// Build a remapped palette for a batch: local bone index â†’ global palette matrix
// Go equivalent: instanceJointsMap[jointIndexes[0]] in export_gltf.go:126
std::vector<glm::mat4> SceneRenderer::BuildBatchPalette(const RenderBatch& batch) const {
    if (batch.jointMap.empty() || m_jointPalette.empty()) return m_jointPalette;

    // The remapped palette has one entry per local joint index.
    // Local index i maps to global index jointMap[i], and we fetch the matrix from m_jointPalette.
    std::vector<glm::mat4> remapped(batch.jointMap.size(), glm::mat4(1.0f));
    for (size_t i = 0; i < batch.jointMap.size(); ++i) {
        uint16_t globalIdx = batch.jointMap[i];
        if (globalIdx < m_jointPalette.size()) {
            remapped[i] = m_jointPalette[globalIdx];
        }
    }
    return remapped;
}

// â”€â”€ Background gradient â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SceneRenderer::RenderBackground(const glm::vec3& topColor, const glm::vec3& bottomColor) {
    auto* shader = ShaderManager::Get().GetShader("background");
    GLuint bgVAO = ShaderManager::Get().m_backgroundVAO;
    if (!shader || !bgVAO) return;

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    shader->Use();
    shader->SetVec3("uTopColor", topColor);
    shader->SetVec3("uBottomColor", bottomColor);

    glBindVertexArray(bgVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

// â”€â”€ Stats â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

int SceneRenderer::GetTotalVertices() const {
    int total = 0;
    for (const auto& b : m_batches)
        if (b.gpuMesh) total += b.gpuMesh->GetVertexCount();
    return total;
}

int SceneRenderer::GetTotalTriangles() const {
    int total = 0;
    for (const auto& b : m_batches)
        if (b.gpuMesh) total += b.gpuMesh->GetIndexCount() / 3;
    return total;
}

// â”€â”€ Render â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SceneRenderer::RenderSky(const glm::mat4& view, const glm::mat4& proj, ShadingMode mode) {
    if (m_skyBatches.empty()) return;

    auto* shader = ShaderManager::Get().GetShader("scene");
    if (!shader) return;

    // Sky rendering: use rotation-only view matrix (strip translation)
    glm::mat4 skyView = glm::mat4(glm::mat3(view)); 

    // Render sky batches
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    RenderBatches(m_skyBatches, shader, skyView, proj, mode);
    
    // Clear depth so scene geometry renders on top of sky
    glClear(GL_DEPTH_BUFFER_BIT);
}

void SceneRenderer::Render(const glm::mat4& view, const glm::mat4& proj, ShadingMode mode, int viewportW, int viewportH) {
    if (m_batches.empty()) return;

    auto* shader = ShaderManager::Get().GetShader("scene");
    if (!shader) return;

    // Compute base shader mode for filled faces
    bool isWireframeMode = (mode == ShadingMode::Wireframe || mode == ShadingMode::TexturedWire);
    ShadingMode effectiveMode = mode;
    if (mode == ShadingMode::Wireframe) effectiveMode = ShadingMode::Matcap;
    if (mode == ShadingMode::TexturedWire) effectiveMode = ShadingMode::Textured;

    // Pass 1: Opaque
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    RenderBatches(m_opaqueBatches, shader, view, proj, effectiveMode);

    // Pass 2: Additive / Translucent
    if (!m_additiveBatches.empty()) {
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL); // Required for coplanar layered geometry (tattoos)
        RenderBatches(m_additiveBatches, shader, view, proj, effectiveMode);
        glDepthFunc(GL_LESS);   // Restore
    }

    // Pass 2b: Wireframe overlay
    if (isWireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        
        shader->Use();
        shader->SetInt("uWireframeOverride", 1);
        auto* cfg = AppConfig::Get();
        glm::vec4 wireColor = cfg ? glm::vec4(cfg->wireR, cfg->wireG, cfg->wireB, 1.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        shader->SetVec4("uWireColor", wireColor);
        
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        RenderBatches(m_opaqueBatches, shader, view, proj, effectiveMode);
        
        if (!m_additiveBatches.empty()) {
            glEnable(GL_BLEND);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            RenderBatches(m_additiveBatches, shader, view, proj, effectiveMode);
            glDepthFunc(GL_LESS);
        }
        
        shader->SetInt("uWireframeOverride", 0);
        glDisable(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Pass 3: Highlight outlines (screen-space)
    RenderOutlineScreenSpace(view, proj, viewportW, viewportH);

    // Pass 4: Cleanup state
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SceneRenderer::RenderBatches(const std::vector<RenderBatch*>& batches,
                                  Shader* shader, const glm::mat4& view,
                                  const glm::mat4& proj, ShadingMode mode) {
    if (batches.empty() || !shader) return;

    shader->Use();
    shader->SetMat4("uModelTransform", m_instanceTransform);
    shader->SetMat4("uView", view);
    shader->SetMat4("uProjection", proj);

    // Shading mode uniform
    int shadingInt = 0;
    switch (mode) {
        case ShadingMode::Solid:     shadingInt = 0; break;
        case ShadingMode::Matcap:    shadingInt = 1; break;
        case ShadingMode::Textured:  shadingInt = 2; break;
        default:                     shadingInt = 0; break;
    }
    shader->SetInt("uShadingMode", shadingInt);

    // Lighting
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.4f, 0.8f, 0.5f));
    shader->SetVec3("uLightDir", lightDir);
    glm::vec3 viewPos = glm::vec3(glm::inverse(view)[3]);
    shader->SetVec3("uViewPos", viewPos);

    // Matcap texture (always bound to unit 2)
    if (mode == ShadingMode::Matcap) {
        GLuint matcapTex = ShaderManager::Get().GetMatcapTexture();
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, matcapTex);
        shader->SetInt("uMatcap", 2);
    }

    // State tracking to minimize GL calls
    GLuint currentTex0 = 0;
    GLuint currentTex1 = 0;

    for (auto* batch : batches) {
        if (!batch->gpuMesh || !batch->isVisible) continue;

        if (batch->blendMode == BlendMode::Additive) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        // Joint palette â€” build a per-batch remapped palette so that
        // vertex local boneIndices directly index into the uploaded array.
        // DEBUG: set m_debugDisableSkin=true to force uUseJoints=0 for all batches
        bool useJoints = !m_debugDisableSkin &&
                         batch->hasSkeleton && m_skeleton && !batch->jointMap.empty();

        shader->SetInt("uUseJoints", useJoints ? 1 : 0);

        if (useJoints && !m_jointPalette.empty()) {
            auto batchPalette = BuildBatchPalette(*batch);
            size_t count = std::min<size_t>(batchPalette.size(), 150);
            glUniformMatrix4fv(glGetUniformLocation(shader->id, "uJoints[0]"),
                               (GLsizei)count, GL_FALSE, glm::value_ptr(batchPalette[0]));
        }

        // Material uniforms
        glUniform4fv(glGetUniformLocation(shader->id, "uMaterialColor"), 1, batch->materialColor);
        glUniform4fv(glGetUniformLocation(shader->id, "uLayerColor"), 1, batch->layerColor);
        glUniform2fv(glGetUniformLocation(shader->id, "uLayerOffset"), 1, batch->uvOffset);
        shader->SetInt("uUseVertexColor", 1);

        // Diffuse texture (unit 0)
        shader->SetInt("uUseTexture", batch->hasTexture ? 1 : 0);
        if (batch->hasTexture && batch->texture0 != currentTex0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, batch->texture0);
            shader->SetInt("uTexture0", 0);
            currentTex0 = batch->texture0;
        }

        // Environment map (unit 1)
        shader->SetInt("uUseEnvmap", batch->hasEnvmap ? 1 : 0);
        if (batch->hasEnvmap && batch->texture1 != currentTex1) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, batch->texture1);
            shader->SetInt("uEnvmap", 1);
            currentTex1 = batch->texture1;
        }

        batch->gpuMesh->Draw();
    }
}

// â”€â”€ Skeleton debug rendering â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SceneRenderer::RenderSkeleton(const glm::mat4& view, const glm::mat4& proj) {
    if (!m_skeleton || m_jointWorldPos.empty()) return;

    struct LineVert { glm::vec3 pos; glm::vec4 color; };
    std::vector<LineVert> lines;

    auto* cfg = AppConfig::Get();
    glm::vec4 boneColor = cfg ? glm::vec4(cfg->boneR, cfg->boneG, cfg->boneB, 1.0f) : glm::vec4(0.0f, 1.0f, 0.4f, 1.0f);
    glm::vec4 rootColor = cfg ? glm::vec4(cfg->boneG, cfg->boneR, cfg->boneB, 1.0f) : glm::vec4(1.0f, 0.3f, 0.1f, 1.0f);
    glm::vec4 jointDot = boneColor * 0.5f + glm::vec4(0.5f);
    jointDot.a = 1.0f;

    for (size_t i = 0; i < m_skeleton->joints.size() && i < m_jointWorldPos.size(); ++i) {
        const auto& joint = m_skeleton->joints[i];

        // Use the world-space rest pose position (NOT from jointPalette which includes bindToJoint)
        glm::vec3 pos = glm::vec3(m_instanceTransform * glm::vec4(m_jointWorldPos[i], 1.0f));

        if (joint.parent >= 0 && joint.parent < (int)m_jointWorldPos.size()) {
            glm::vec3 parentPos = glm::vec3(m_instanceTransform * glm::vec4(m_jointWorldPos[joint.parent], 1.0f));
            lines.push_back({parentPos, boneColor});
            lines.push_back({pos, boneColor});
        } else {
            float s = 0.05f;
            lines.push_back({pos + glm::vec3(-s, 0, 0), rootColor});
            lines.push_back({pos + glm::vec3( s, 0, 0), rootColor});
            lines.push_back({pos + glm::vec3(0, -s, 0), rootColor});
            lines.push_back({pos + glm::vec3(0,  s, 0), rootColor});
            lines.push_back({pos + glm::vec3(0, 0, -s), rootColor});
            lines.push_back({pos + glm::vec3(0, 0,  s), rootColor});
        }

        float d = 0.02f;
        lines.push_back({pos + glm::vec3(-d, 0, 0), jointDot});
        lines.push_back({pos + glm::vec3( d, 0, 0), jointDot});
        lines.push_back({pos + glm::vec3(0, -d, 0), jointDot});
        lines.push_back({pos + glm::vec3(0,  d, 0), jointDot});

        // â”€â”€ Per-joint orientation axes (X=red, Y=green, Z=blue) â”€â”€â”€â”€â”€â”€â”€â”€â”€
        // Extract basis vectors from the joint's world rest matrix.
        // This shows whether the bone's local frame is rotated correctly
        // relative to the mesh (e.g. palm direction).
        glm::mat4 worldMat = m_instanceTransform * joint.renderMat;
        glm::vec3 ax = glm::vec3(worldMat[0]);
        glm::vec3 ay = glm::vec3(worldMat[1]);
        glm::vec3 az = glm::vec3(worldMat[2]);
        float aLen = 0.04f;
        glm::vec4 axR(1, 0.2f, 0.2f, 1);
        glm::vec4 axG(0.2f, 1, 0.2f, 1);
        glm::vec4 axB(0.3f, 0.5f, 1, 1);
        lines.push_back({pos, axR}); lines.push_back({pos + glm::normalize(ax) * aLen, axR});
        lines.push_back({pos, axG}); lines.push_back({pos + glm::normalize(ay) * aLen, axG});
        lines.push_back({pos, axB}); lines.push_back({pos + glm::normalize(az) * aLen, axB});
    }

    if (lines.empty()) return;

    auto* shader = ShaderManager::Get().GetShader("grid");
    if (!shader) return;

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(LineVert), lines.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVert), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVert), (void*)offsetof(LineVert, color));
    glEnableVertexAttribArray(1);

    shader->Use();
    shader->SetMat4("uView", view);
    shader->SetMat4("uProjection", proj);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    glDrawArrays(GL_LINES, 0, (GLsizei)lines.size());

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(0);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

// â”€â”€ Clear â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SceneRenderer::Clear() {
    if (m_maskFbo) { glDeleteFramebuffers(1, &m_maskFbo); m_maskFbo = 0; }
    if (m_maskTex) { glDeleteTextures(1, &m_maskTex); m_maskTex = 0; }
    if (m_maskDepth) { glDeleteRenderbuffers(1, &m_maskDepth); m_maskDepth = 0; }
    m_maskW = m_maskH = 0;

    m_batches.clear();
    m_opaqueBatches.clear();
    m_additiveBatches.clear();
    m_skyBatches.clear();
    m_hasSky = false;
    m_skeleton.reset();
    m_jointPalette.clear();
    m_jointWorldPos.clear();
    m_diagDone = false;

    for (GLuint tex : m_ownedTextures) {
        if (tex) glDeleteTextures(1, &tex);
    }
    m_ownedTextures.clear();

    m_bounds = {};

    m_animData.reset();
    m_animPlayer.reset();
}

void SceneRenderer::EnsureMaskFBO(int w, int h) {
    if (w == m_maskW && h == m_maskH && m_maskFbo != 0) return;
    m_maskW = w; m_maskH = h;

    if (m_maskFbo) { glDeleteFramebuffers(1, &m_maskFbo); m_maskFbo = 0; }
    if (m_maskTex) { glDeleteTextures(1, &m_maskTex); m_maskTex = 0; }
    if (m_maskDepth) { glDeleteRenderbuffers(1, &m_maskDepth); m_maskDepth = 0; }

    glGenFramebuffers(1, &m_maskFbo);
    glGenTextures(1, &m_maskTex);
    glGenRenderbuffers(1, &m_maskDepth);

    glBindTexture(GL_TEXTURE_2D, m_maskTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, m_maskDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

    glBindFramebuffer(GL_FRAMEBUFFER, m_maskFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_maskTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_maskDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneRenderer::RenderOutlineScreenSpace(const glm::mat4& view, const glm::mat4& proj,
                                             int viewportW, int viewportH) {
    // Check if any batch is highlighted
    bool hasHighlight = false;
    for (const auto& batch : m_batches) {
        if (batch.isHighlighted && batch.isVisible && batch.gpuMesh) {
            hasHighlight = true; break;
        }
    }
    if (!hasHighlight) return;

    // --- Pass A: Render highlighted meshes to mask FBO ---
    EnsureMaskFBO(viewportW, viewportH);

    auto* maskShader = ShaderManager::Get().GetShader("outline_mask");
    if (!maskShader) return;

    GLint prevFbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

    glBindFramebuffer(GL_FRAMEBUFFER, m_maskFbo);
    glViewport(0, 0, viewportW, viewportH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    maskShader->Use();
    maskShader->SetMat4("uModelTransform", m_instanceTransform);
    maskShader->SetMat4("uView", view);
    maskShader->SetMat4("uProjection", proj);

    for (const auto& batch : m_batches) {
        if (!batch.isHighlighted || !batch.isVisible || !batch.gpuMesh) continue;

        bool useJoints = batch.hasSkeleton && m_skeleton && !batch.jointMap.empty();
        maskShader->SetInt("uUseJoints", useJoints ? 1 : 0);
        if (useJoints && !m_jointPalette.empty()) {
            auto batchPalette = BuildBatchPalette(batch);
            size_t count = std::min<size_t>(batchPalette.size(), 150);
            glUniformMatrix4fv(glGetUniformLocation(maskShader->id, "uJoints[0]"),
                               (GLsizei)count, GL_FALSE, glm::value_ptr(batchPalette[0]));
        }

        // Alpha test support
        maskShader->SetInt("uUseTexture", batch.hasTexture ? 1 : 0);
        if (batch.hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, batch.texture0);
            maskShader->SetInt("uTexture0", 0);
        }

        batch.gpuMesh->Draw();
    }

    // --- Pass B: Post-process outline detection ---
    glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    glViewport(0, 0, viewportW, viewportH);

    auto* postShader = ShaderManager::Get().GetShader("outline_post");
    if (!postShader) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    postShader->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_maskTex);
    postShader->SetInt("uMaskTex", 0);
    postShader->SetVec2("uTexelSize", glm::vec2(1.0f / viewportW, 1.0f / viewportH));

    auto* cfg = AppConfig::Get();
    glm::vec4 hlColor = cfg ? glm::vec4(cfg->hlR, cfg->hlG, cfg->hlB, cfg->hlA)
                            : glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    postShader->SetVec4("uOutlineColor", hlColor);
    postShader->SetFloat("uOutlineWidth", 1.8f);

    GLuint bgVAO = ShaderManager::Get().m_backgroundVAO;
    glBindVertexArray(bgVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

} // namespace Onyx
