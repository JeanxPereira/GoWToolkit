#include "core/loaders/GOW2Loaders.h"
#include "ui/viewers/Viewport3D.h"
#include "ui/viewers/ImageViewer.h"
#include "ui/viewers/MaterialViewer.h"
#include "ui/viewers/SoundPlayer.h"
#include "ui/viewers/VideoPlayer.h"
#include "core/parsers/gow2/MeshParser.h"
#include "core/parsers/gow2/TextureParser.h"
#include "core/parsers/gow2/MaterialParser.h"
#include "core/parsers/gow2/SoundParser.h"
#include "core/parsers/gow2/VagParser.h"
#include "core/parsers/gow2/VpkParser.h"
#include "core/vfs/SliceFile.h"

namespace GOW {

static const ParsedEntry* FindEntryWithPayload(const std::vector<ParsedEntry>& entries, const std::string& name, const std::string& type) {
    for (const auto& entry : entries) {
        if (entry.schemaType == type && entry.name == name && entry.size > 0)
            return &entry;
        if (!entry.children.empty()) {
            if (auto found = FindEntryWithPayload(entry.children, name, type))
                return found;
        }
    }
    return nullptr;
}

// ── GOW2ModelLoader ──────────────────────────────────────────────────────────

bool GOW2ModelLoader::canHandle(const std::string& schemaType) const {
    return schemaType == "GOW2_MDL" || schemaType == "GOW2_SERVER_INSTANCE" || schemaType == "GOW2_OBJ";
}

std::shared_ptr<IDocumentContent> GOW2ModelLoader::load(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;

    auto vp = std::make_shared<GOW::Viewport3D>(entry.name);
    const ParsedEntry* meshEntry = nullptr;
    std::vector<const ParsedEntry*> matEntries;

    std::string baseName = entry.name;
    if (baseName.find("obj_") == 0) baseName = baseName.substr(4);
    else if (baseName.find("mdl_") == 0) baseName = baseName.substr(4);
    else if (baseName.find("mesh_") == 0) baseName = baseName.substr(5);

    std::string targetMdl = "mdl_" + baseName;
    std::string targetMesh = "mesh_" + baseName;
    std::string targetObj = "obj_" + baseName;

    // A helper to traverse the entire WAD if necessary
    std::function<void(const std::vector<ParsedEntry>&)> searchWad = [&](const std::vector<ParsedEntry>& nodes) {
        for (const auto& child : nodes) {
            if (!meshEntry && (child.schemaType == "GOW2_MESH" || child.schemaType == "GOW2_MDL")) {
                if (child.name == entry.name || child.name == targetMdl || child.name == targetMesh || child.name == baseName) {
                    meshEntry = &child;
                }
            } else if (child.schemaType == "GOW2_MAT") {
                const ParsedEntry* matEntry = &child;
                if (matEntry->size == 0) {
                    if (auto realMat = FindEntryWithPayload(wad.entries, matEntry->name, "GOW2_MAT"))
                        matEntry = realMat;
                }
                // Try to heuristically add materials that belong to this model or global SKSText
                // But GOW2 mesh natively points to material IDs. We just collect all valid ones.
                matEntries.push_back(matEntry);
            }
            searchWad(child.children);
        }
    };

    if (entry.schemaType == "GOW2_SERVER_INSTANCE" || entry.schemaType == "GOW2_OBJ") {
        searchWad(wad.entries); // global scan for related
    } else if (!entry.children.empty()) {
        searchWad(entry.children); // fallback local scan
        if (!meshEntry) searchWad(wad.entries); // global scan if not found
    } else {
        meshEntry = &entry;
        searchWad(wad.entries); // globally gather materials
    }

    if (meshEntry) {
        GOW::SliceFile slice(wad.fileSource, meshEntry->offset, meshEntry->size);
        auto meshData = GOW::GOW2MeshParser::Parse(slice, 0, meshEntry->size);
        if (meshData && !meshData->parts.empty()) {
            std::vector<std::unique_ptr<GOW::TextureData>> textures;
            for (const auto* matEntry : matEntries) {
                auto matData = GOW::GOW2MaterialParser::Parse(*matEntry, wad.fileSource);
                std::unique_ptr<GOW::TextureData> texData = nullptr;
                if (matData && !matData->layers.empty()) {
                    for (const auto& layer : matData->layers) {
                        if (layer.hasTexture && !layer.textureName.empty()) {
                            for (const auto& c : wad.entries) {
                                if (c.schemaType == "GOW2_TXR" && c.name == layer.textureName) {
                                    texData = GOW::GOW2TextureParser::Parse(c, wad.entries, wad.fileSource);
                                    break;
                                }
                            }
                            if (texData) break;
                        }
                    }
                }
                textures.push_back(std::move(texData));
            }
            vp->LoadFromMeshData(*meshData, textures);
        }
    }
    return vp;
}

// ── GOW2MeshLoader ───────────────────────────────────────────────────────────

bool GOW2MeshLoader::canHandle(const std::string& schemaType) const {
    return schemaType == "GOW2_MESH";
}

std::shared_ptr<IDocumentContent> GOW2MeshLoader::load(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;
    auto vp = std::make_shared<GOW::Viewport3D>(entry.name);
    GOW::SliceFile slice(wad.fileSource, entry.offset, entry.size);
    auto meshData = GOW::GOW2MeshParser::Parse(slice, 0, entry.size);
    if (meshData && !meshData->parts.empty()) {
        std::vector<std::unique_ptr<GOW::TextureData>> textures;
        vp->LoadFromMeshData(*meshData, textures);
    }
    return vp;
}

// ── GOW2TextureLoader ────────────────────────────────────────────────────────

bool GOW2TextureLoader::canHandle(const std::string& schemaType) const {
    return schemaType == "GOW2_TXR";
}

std::shared_ptr<IDocumentContent> GOW2TextureLoader::load(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;
    auto texData = GOW::GOW2TextureParser::Parse(entry, wad.entries, wad.fileSource);
    if (texData && texData->IsValid())
        return std::make_shared<GOW::ImageViewer>(entry.name, std::move(texData));
    return nullptr;
}

// ── GOW2MaterialLoader ───────────────────────────────────────────────────────

bool GOW2MaterialLoader::canHandle(const std::string& schemaType) const {
    return schemaType == "GOW2_MAT";
}

std::shared_ptr<IDocumentContent> GOW2MaterialLoader::load(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;
    
    const ParsedEntry* matEntryToParse = &entry;
    if (matEntryToParse->size == 0) {
        if (auto realMat = FindEntryWithPayload(wad.entries, matEntryToParse->name, "GOW2_MAT"))
            matEntryToParse = realMat;
    }

    auto matData = GOW::GOW2MaterialParser::Parse(*matEntryToParse, wad.fileSource);

    if (matData) {
        return std::make_shared<GOW::MaterialViewer>(
            entry.name, 
            std::move(matData), 
            [](const std::string& texName) -> unsigned int {
                return 0; 
            }
        );
    }
    return nullptr;
}

// ── GOW2SoundLoader ──────────────────────────────────────────────────────────

bool GOW2SoundLoader::canHandle(const std::string& schemaType) const {
    return schemaType == "GOW2_SFX";
}

std::shared_ptr<IDocumentContent> GOW2SoundLoader::load(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;
    auto bankData = GOW::GOW2SoundParser::Parse(entry, wad.fileSource);
    if (bankData && !bankData->sounds.empty())
        return std::make_shared<GOW::SoundPlayer>(entry.name, std::move(bankData));
    return nullptr;
}

// ── GOW2VagLoader ────────────────────────────────────────────────────────────

bool GOW2VagLoader::canHandle(const std::string& schemaType) const {
    return schemaType == "GOW2_VAG";
}

std::shared_ptr<IDocumentContent> GOW2VagLoader::load(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;
    auto vagData = GOW::GOW2VagParser::Parse(wad.fileSource);
    if (vagData && !vagData->pcmData.empty())
        return std::make_shared<GOW::SoundPlayer>(entry.name, std::move(vagData->pcmData), vagData->sampleRate, vagData->channels);
    return nullptr;
}

// ── GOW2VpkLoader ────────────────────────────────────────────────────────────

bool GOW2VpkLoader::canHandle(const std::string& schemaType) const {
    return schemaType == "GOW2_VPK";
}

std::shared_ptr<IDocumentContent> GOW2VpkLoader::load(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;
    auto vpkData = GOW::GOW2VpkParser::Parse(wad.fileSource);
    if (vpkData && !vpkData->pcmData.empty())
        return std::make_shared<GOW::SoundPlayer>(entry.name, std::move(vpkData->pcmData), vpkData->sampleRate, vpkData->channels);
    return nullptr;
}

// ── GOW2PssLoader ────────────────────────────────────────────────────────────
// Handles PSS (FMV cutscenes) and PSW (PSS variant).
// Both are MPEG-2 Program Streams — FFmpeg opens them natively.
// The file lives inside a PAK/ISO so we pass the IFile directly via AVIO;
// no temp file extraction is needed.

bool GOW2PssLoader::canHandle(const std::string& schemaType) const {
    return schemaType == "GOW2_PSS" || schemaType == "GOW2_PSW";
}

std::shared_ptr<IDocumentContent> GOW2PssLoader::load(const ParsedEntry& entry, OpenWad& wad) {
    if (!wad.fileSource) return nullptr;

    // Wrap the relevant byte range so the AVIO cursor stays within this file
    auto slice = std::make_shared<GOW::SliceFile>(wad.fileSource, entry.offset, entry.size);
    return std::make_shared<GOW::VideoPlayer>(entry.name, slice);
}

} // namespace GOW