#pragma once
#include <string>
#include <vector>
#include <memory>
#include "schema/NodeInstance.h"

namespace GOW {
    class IGameProfile;
    class IFile;
}

struct ParsedEntry {
    std::string           name;
    std::string           wadName;
    uint32_t              size = 0;
    uint32_t              offset = 0;
    uint64_t              hash = 0; // Added back for UI
    
    // Identificado pelo schema (ex: "GOW2_MDL", "GOW1_TXR")
    std::string           schemaType; 
    
    // Sub-nodos para formar uma árvore de UI
    std::vector<ParsedEntry> children;
    
    // Dados carregados (quando solicitados)
    std::shared_ptr<GOW::NodeInstance> instance;
};

struct OpenWad {
    std::string                        filename;
    std::string                        fullPath;
    std::shared_ptr<GOW::IGameProfile> profile;
    std::shared_ptr<GOW::IFile>        fileSource;
    std::vector<ParsedEntry>           entries;
};
