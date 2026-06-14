// Remaining Onyx content type handlers.
// Each registers by magic number for GOW1 and/or GOW2.

#include "core/formats/GOW2AnimationFormat.h"
#include <Onyx/Schema/AssetReader.h>
#include <Onyx/Types/ITypeHandler.h>
#include <Onyx/Types/TypeRegistry.h>
#include "core/types/GameTypes.h"
#include <Onyx/Fonts/SFSymbols.h>
#include "ui/viewers/SoundPlayer.h"

// Parsers
#include <Onyx/Services/Logger.h>
#include "core/parsers/gow2/InstanceParser.h"
#include "core/parsers/gow2/SoundParser.h"
#include "core/parsers/gow2/VagParser.h"
#include "core/parsers/gow2/VpkParser.h"
#include <Onyx/Parsers/SceneNode.h>
#include <Onyx/Vfs/IFile.h>
#include <Onyx/Vfs/SliceFile.h>
#include <Onyx/Viewers/VideoPlayer.h>
#include <Onyx/Viewers/Viewport3D.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <functional>

namespace {

// Ã¢â€â‚¬Ã¢â€â‚¬ Animation Ã¢â€â‚¬Ã¢â€â‚¬ magic 0x00000003 (ANIMATIONS_MAGIC)
class AnimationHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::Animation; }
  const char *GetName() const override { return "Animation"; }
  uint32_t GetMagic() const override { return 0x00000003; }
  const char *GetIcon() const override { return ICON_SF_PLAY_FILL; } // play
  Color4f GetColor() const override {
    return {1.0f, 0.8f, 0.3f, 1.0f};
  } // amarelo

  std::shared_ptr<Onyx::Schema::AssetNode>
  Parse(std::shared_ptr<Onyx::Vfs::IFile> file) override {
    if (!file || file->Size() < 32)
      return nullptr;
    Onyx::GOW2AnimationFormat format;
    format.Initialize();
    return Onyx::Schema::AssetReader::Parse(*format.Root(), file);
  }
};

// Ã¢â€â‚¬Ã¢â€â‚¬ Script Ã¢â€â‚¬Ã¢â€â‚¬ magic 0x00010004 (SCRIPT_MAGIC)
class ScriptHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::Script; }
  const char *GetName() const override { return "Script"; }
  uint32_t GetMagic() const override { return 0x00010004; }
  const char *GetIcon() const override {
    return ICON_SF_CHEVRON_LEFT_FORWARDSLASH_CHEVRON_RIGHT;
  } // code
  Color4f GetColor() const override {
    return {0.5f, 1.0f, 0.5f, 1.0f};
  } // green
};

// Ã¢â€â‚¬Ã¢â€â‚¬ Light Ã¢â€â‚¬Ã¢â€â‚¬ magic 0x00000006 (LIGHT_MAGIC)
class LightHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::Light; }
  const char *GetName() const override { return "Light"; }
  uint32_t GetMagic() const override { return 0x00000006; }
  const char *GetIcon() const override { return ICON_SF_SPARKLES; } // sparkle
  Color4f GetColor() const override {
    return {1.0f, 1.0f, 0.6f, 1.0f};
  } // yellow
};

// Ã¢â€â‚¬Ã¢â€â‚¬ Sound (GOW2) Ã¢â€â‚¬Ã¢â€â‚¬ magic 0x00000015 (GOW2_SBP_MAGIC)
class SoundHandlerGOW2 : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::Sound; }
  const char *GetName() const override { return "Sound"; }
  uint32_t GetMagic() const override { return 0x00000015; }
  const char *GetIcon() const override {
    return ICON_SF_SPEAKER_WAVE_2_FILL;
  } // unmute
  Color4f GetColor() const override { return {0.3f, 0.9f, 0.6f, 1.0f}; } // teal

  std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry &entry,
                                                      AssetContainer &wad) override {
    if (!wad.fileSource)
      return nullptr;
    auto bankData = Onyx::GOW2SoundParser::Parse(entry, wad.fileSource);
    if (bankData && !bankData->sounds.empty())
      return std::make_shared<Onyx::SoundPlayer>(entry.name,
                                                std::move(bankData));
    return nullptr;
  }
};

// Ã¢â€â‚¬Ã¢â€â‚¬ Collision Ã¢â€â‚¬Ã¢â€â‚¬ magic 0x00000011 (COLLISION_MAGIC)
class CollisionHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::Collision; }
  const char *GetName() const override { return "Collision"; }
  uint32_t GetMagic() const override { return 0x00000011; }
  Color4f GetColor() const override { return {0.7f, 0.7f, 0.7f, 1.0f}; }
};

// Ã¢â€â‚¬Ã¢â€â‚¬ Flipbook (GOW2) Ã¢â€â‚¬Ã¢â€â‚¬ magic 0x0000001B
class FlipbookHandlerGOW2 : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::Flipbook; }
  const char *GetName() const override { return "Flipbook"; }
  uint32_t GetMagic() const override { return 0x0000001B; }
  Color4f GetColor() const override { return {1.0f, 0.6f, 0.9f, 1.0f}; }
};

// Ã¢â€â‚¬Ã¢â€â‚¬ Chunk Ã¢â€â‚¬Ã¢â€â‚¬ magic 0x80000001 (CHUNK_MAGIC / context)
class ChunkHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::Chunk; }
  const char *GetName() const override { return "Chunk"; }
  uint32_t GetMagic() const override { return 0x80000001; }
  Color4f GetColor() const override { return {0.6f, 0.6f, 0.6f, 1.0f}; }

  std::unique_ptr<Onyx::Parsers::SceneData> BuildSceneData(const AssetEntry &entry,
                                                 AssetContainer &wad) override {
    // A Chunk aggregates instances. Delegate to InstanceHandler and merge
    // results.
    auto mergedScene = std::make_unique<Onyx::Parsers::SceneData>();
    int instanceCount = 0;

    LOG_INFO("[ChunkHandler] BuildSceneData started for chunk '%s'",
             entry.name.c_str());

    // Resolve the Instance handler once
    auto *instHandler = Onyx::Types::TypeRegistry::Get().Resolve(Onyx::GameTypes::Instance);

    auto findInstances = [&](const std::vector<AssetEntry> &entries,
                             auto &findRef) -> void {
      for (const auto &child : entries) {
        if (child.typeId == Onyx::GameTypes::Instance && instHandler) {
          LOG_INFO("[ChunkHandler] Found instance '%s'", child.name.c_str());

          // Delegate to InstanceHandler::BuildSceneData
          // (parses transform, resolves Object/Model child, applies transform)
          if (auto instScene = instHandler->BuildSceneData(child, wad)) {
            LOG_INFO("[ChunkHandler] Got SceneData for instance '%s' "
                     "(meshes=%zu, isSky=%d, hasSkeleton=%d)",
                     child.name.c_str(), instScene->meshParts.size(),
                     instScene->isSky, instScene->HasSkeleton());

            // Bake skeleton joint world-rest pose into vertices, then strip
            // skeleton so chunk merge can flatten many instances into one
            // SceneData.
            //
            // Critical reference (god_of_war_browser/web/.../BrowserWad.js:1365):
            //   if (inst.IsGow2) {
            //     // instNode.setLocalMatrix(instMat);  Ã¢â€ Â COMMENTED OUT
            //   }
            // For GOW2, `inst.Position` is NOT applied to the rendered object Ã¢â‚¬â€
            // the joint world transforms (Matrixes1 chain via renderMat) already
            // place the geometry in world space. Applying instance translation
            // on top double-counts the position.
            //
            // Port of web renderer skinning (SkinnedTextured.vs):
            //   boneTransform = umJoints[id1] * w + umJoints[id2] * (1 - w)
            //   pos           = boneTransform * pos
            // where umJoints[i] = joint.renderMat * joint.bindToJointMat.
            //
            // Sky instances: same rule Ã¢â‚¬â€ joint world transform only.
            {
              std::vector<glm::mat4> palette;
              if (instScene->HasSkeleton()) {
                const auto &joints = instScene->skeleton->joints;
                palette.resize(joints.size());
                for (size_t i = 0; i < joints.size(); ++i) {
                  palette[i] = joints[i].renderMat * joints[i].bindToJointMat;
                }
              }

              for (auto &part : instScene->meshParts) {
                std::vector<glm::mat4> batchPalette;
                if (!palette.empty() && !part.jointMap.empty()) {
                  batchPalette.resize(part.jointMap.size(), glm::mat4(1.0f));
                  for (size_t i = 0; i < part.jointMap.size(); ++i) {
                    uint16_t g = part.jointMap[i];
                    if (g < palette.size()) {
                      batchPalette[i] = palette[g];
                    }
                  }
                }

                for (auto &v : part.vertices) {
                  glm::mat4 boneT(1.0f);
                  if (!batchPalette.empty()) {
                    uint32_t i1 = v.boneIndices.x;
                    uint32_t i2 = v.boneIndices.y;
                    float w1 = v.boneWeights.x;
                    float w2 = v.boneWeights.y;
                    if (w1 == 0.0f && w2 == 0.0f) {
                      w1 = 1.0f;
                    }
                    glm::mat4 M1 = i1 < batchPalette.size() ? batchPalette[i1]
                                                            : glm::mat4(1.0f);
                    glm::mat4 M2 = i2 < batchPalette.size() ? batchPalette[i2]
                                                            : glm::mat4(1.0f);
                    boneT = M1 * w1 + M2 * w2;
                  }

                  glm::mat3 M3(boneT);

                  glm::vec4 pos(v.position[0], v.position[1], v.position[2], 1.0f);
                  pos = boneT * pos;
                  v.position[0] = pos.x;
                  v.position[1] = pos.y;
                  v.position[2] = pos.z;

                  glm::vec3 n3(v.normal[0], v.normal[1], v.normal[2]);
                  n3 = M3 * n3;
                  float nlen = glm::length(n3);
                  if (nlen > 1e-6f) n3 /= nlen;
                  v.normal[0] = n3.x;
                  v.normal[1] = n3.y;
                  v.normal[2] = n3.z;

                  v.boneIndices = glm::uvec4(0u);
                  v.boneWeights = glm::vec4(0.0f);
                }
                part.jointMap.clear();
              }

              instScene->skeleton.reset();
              instScene->instanceTransform = glm::mat4(1.0f);
              LOG_INFO("[ChunkHandler] Baked instance '%s' to world space (isSky=%d)",
                       child.name.c_str(), instScene->isSky);
            }

            // Merge materials
            uint32_t materialOffset = mergedScene->materials.size();
            for (auto &mat : instScene->materials) {
              mergedScene->materials.push_back(std::move(mat));
            }
            // Merge textures
            for (auto &tx : instScene->textures) {
              mergedScene->textures.push_back(std::move(tx));
            }
            // Merge mesh parts (adjust material IDs)
            for (auto &part : instScene->meshParts) {
              part.materialId += materialOffset;
              mergedScene->meshParts.push_back(std::move(part));
            }

            // Propagate sky flag so MapViewer's combined scene can route the
            // dome through RenderSky.
            if (instScene->isSky) {
              mergedScene->isSky = true;
            }

            instanceCount++;
          } else {
            LOG_WARN("[ChunkHandler] InstanceHandler returned null for '%s'",
                     child.name.c_str());
          }
        }

        if (!child.children.empty()) {
          findRef(child.children, findRef);
        }
      }
    };

    findInstances(entry.children, findInstances);

    LOG_INFO("[ChunkHandler] BuildSceneData completed. Found %d instances.",
             instanceCount);
    if (instanceCount > 0) {
      return mergedScene;
    }
    return nullptr;
  }

  std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry &entry,
                                                      AssetContainer &wad) override {
    if (auto scene = BuildSceneData(entry, wad)) {
      auto vp = std::make_shared<Onyx::Viewers::Viewport3D>(entry.name);
      vp->LoadScene(std::move(scene));
      return vp;
    }
    return nullptr;
  }
};

// Ã¢â€â‚¬Ã¢â€â‚¬ Shader Group Ã¢â€â‚¬Ã¢â€â‚¬ magic 0x00000027 (SHG_MAGIC)
// Only GOW1
class ShaderGroupHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::ShaderContainer; }
  const char *GetName() const override { return "Shader Group"; }
  uint32_t GetMagic() const override { return 0x00000027; }
  const char *GetIcon() const override {
    return ICON_SF_CHEVRON_LEFT_FORWARDSLASH_CHEVRON_RIGHT;
  }
  Color4f GetColor() const override { return {0.5f, 1.0f, 0.5f, 1.0f}; }
};

// Ã¢â€â‚¬Ã¢â€â‚¬ Audio/Video (File level) Ã¢â€â‚¬Ã¢â€â‚¬
class VagHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::VagAudio; }
  const char *GetName() const override { return "VAG Audio"; }
  uint32_t GetMagic() const override {
    return 0x00;
  } // no magic, extension based
  const char *GetIcon() const override { return ICON_SF_SPEAKER_WAVE_3; }
  Color4f GetColor() const override { return {0.3f, 0.9f, 0.6f, 1.0f}; }

  std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry &entry,
                                                      AssetContainer &wad) override {
    if (!wad.fileSource)
      return nullptr;
    auto vagData = Onyx::GOW2VagParser::Parse(wad.fileSource);
    if (vagData && !vagData->pcmData.empty())
      return std::make_shared<Onyx::SoundPlayer>(
          entry.name, std::move(vagData->pcmData), vagData->sampleRate,
          vagData->channels);
    return nullptr;
  }
};

class VpkHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::VpkVideo; }
  const char *GetName() const override { return "VPK Video"; }
  uint32_t GetMagic() const override { return 0x00; }
  const char *GetIcon() const override { return ICON_SF_SPEAKER_WAVE_2_FILL; }
  Color4f GetColor() const override { return {0.3f, 0.9f, 0.6f, 1.0f}; }

  std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry &entry,
                                                      AssetContainer &wad) override {
    if (!wad.fileSource)
      return nullptr;
    auto vpkData = Onyx::GOW2VpkParser::Parse(wad.fileSource);
    if (vpkData && !vpkData->pcmData.empty())
      return std::make_shared<Onyx::SoundPlayer>(
          entry.name, std::move(vpkData->pcmData), vpkData->sampleRate,
          vpkData->channels);
    return nullptr;
  }
};

class PssHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::PssVideo; }
  const char *GetName() const override { return "PSS Video"; }
  uint32_t GetMagic() const override { return 0x00; }
  const char *GetIcon() const override { return ICON_SF_PLAY_FILL; }
  Color4f GetColor() const override {
    return {0.8f, 0.5f, 0.9f, 1.0f};
  } // purple

  std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry &entry,
                                                      AssetContainer &wad) override {
    if (!wad.fileSource)
      return nullptr;
    auto slice = std::make_shared<Onyx::Vfs::SliceFile>(wad.fileSource, entry.offset,
                                                  entry.size);
    return std::make_shared<Onyx::Viewers::VideoPlayer>(entry.name, slice);
  }
};

class PswHandler : public Onyx::Types::ITypeHandler {
public:
  Onyx::Types::TypeId GetId() const override { return Onyx::GameTypes::PswVideo; }
  const char *GetName() const override { return "PSW Video"; }
  uint32_t GetMagic() const override { return 0x00; }
  const char *GetIcon() const override { return ICON_SF_PLAY_FILL; }
  Color4f GetColor() const override { return {0.8f, 0.5f, 0.9f, 1.0f}; }

  std::shared_ptr<Onyx::Viewers::IDocumentContent> CreateViewer(const AssetEntry &entry,
                                                      AssetContainer &wad) override {
    if (!wad.fileSource)
      return nullptr;
    auto slice = std::make_shared<Onyx::Vfs::SliceFile>(wad.fileSource, entry.offset,
                                                  entry.size);
    return std::make_shared<Onyx::Viewers::VideoPlayer>(entry.name, slice);
  }
};

} // anonymous namespace

// GOW2 registrations (magic-based, WAD internal types)
REGISTER_TYPE(GOW2, AnimationHandler);
REGISTER_TYPE(GOW2, ScriptHandler);
REGISTER_TYPE(GOW2, LightHandler);
REGISTER_TYPE(GOW2, SoundHandlerGOW2);
REGISTER_TYPE(GOW2, FlipbookHandlerGOW2);
REGISTER_TYPE(GOW2, ChunkHandler);

// File-level handlers (identified by extension in TOC/PAK, not by magic).
// Registered by TypeId only Ã¢â‚¬â€ avoids the magic=0x00 collision.
REGISTER_FILE_TYPE(VagHandler);
REGISTER_FILE_TYPE(VpkHandler);
REGISTER_FILE_TYPE(PssHandler);
REGISTER_FILE_TYPE(PswHandler);

