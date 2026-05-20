#include "MapViewer.h"
#include <imgui.h>
#include "core/Logger.h"
#include "core/types/TypeRegistry.h"
#include "core/types/TypeId.h"
#include "core/parsers/shared/SceneNode.h"
#include "core/WadTypes.h"
#include <string>

namespace GOW {

MapViewer::MapViewer(const std::string& wadName, OpenWad& wad) 
    : m_wadName(wadName), m_wad(wad) {
    m_viewport = std::make_unique<Viewport3D>("Map Viewport");
    LoadMap();
}

void MapViewer::Draw() {
    if (m_isLoading) {
        ImGui::Text("Loading Map... %.1f%%", m_loadProgress * 100.0f);
        ImGui::TextDisabled("%s", m_loadStatus.c_str());
        return;
    }
    
    if (m_isLoaded && m_viewport) {
        m_viewport->Draw();
    } else {
        ImGui::TextDisabled("Failed to load map or map is empty.");
    }
}

void MapViewer::DrawInspector() {
    if (m_viewport) {
        m_viewport->DrawInspector();
    }
}

void MapViewer::LoadMap() {
    m_isLoading = true;
    m_loadStatus = "Finding chunks...";
    
    auto combinedScene = std::make_unique<SceneData>();
    
    // 1. Find all CXT_* chunks anywhere in the WAD
    std::vector<const ParsedEntry*> chunks;
    auto findChunks = [&](const std::vector<ParsedEntry>& entries, auto& findRef) -> void {
        for (const auto& entry : entries) {
            if (entry.typeId == TypeId::Chunk && entry.name.find("CXT_") == 0) {
                chunks.push_back(&entry);
            }
            if (!entry.children.empty()) {
                findRef(entry.children, findRef);
            }
        }
    };
    findChunks(m_wad.entries, findChunks);
    
    LOG_INFO("[MapViewer] Found %zu chunks in %s", chunks.size(), m_wadName.c_str());
    
    // 2. Load each chunk into SceneData and merge
    size_t processed = 0;
    for (const auto* chunk : chunks) {
        m_loadStatus = "Loading chunk: " + chunk->name;
        m_loadProgress = (float)processed / (float)chunks.size();
        
        auto handler = TypeRegistry::Get().Resolve(TypeId::Chunk);
        if (handler) {
            auto printTree = [&](const ParsedEntry& e, int depth, auto& pr) -> void {
                std::string indent(depth * 2, ' ');
                LOG_INFO("[MapViewer] %s- %s (type=%d, children=%zu)", indent.c_str(), e.name.c_str(), (int)e.typeId, e.children.size());
                for (const auto& c : e.children) {
                    pr(c, depth + 1, pr);
                }
            };
            LOG_INFO("[MapViewer] Printing Chunk Tree for '%s':", chunk->name.c_str());
            printTree(*chunk, 0, printTree);

            if (auto chunkScene = handler->BuildSceneData(*chunk, m_wad)) {
                // Merge into combined scene
                for (auto& part : chunkScene->meshParts) {
                    part.materialId += combinedScene->materials.size();
                    combinedScene->meshParts.push_back(std::move(part));
                }
                for (auto& mat : chunkScene->materials) {
                    combinedScene->materials.push_back(std::move(mat));
                }
                for (auto& texList : chunkScene->textures) {
                    combinedScene->textures.push_back(std::move(texList));
                }
                
                // Inherit sky flag
                if (chunkScene->isSky) {
                    combinedScene->isSky = true;
                }
            }
        }
        processed++;
    }
    
    m_loadStatus = "Finished";
    m_loadProgress = 1.0f;
    m_isLoading = false;
    m_isLoaded = true;
    
    if (!combinedScene->IsEmpty()) {
        m_viewport->LoadScene(std::move(combinedScene));
    }
}

} // namespace GOW
