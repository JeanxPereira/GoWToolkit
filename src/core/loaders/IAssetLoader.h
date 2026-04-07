#pragma once
#include "core/WadTypes.h"
#include <memory>
#include <string>

// Forward declarations
namespace GOW { class IDocumentContent; }

class IAssetLoader {
public:
    virtual ~IAssetLoader() = default;

    // Which schemaTypes this loader handles
    virtual bool canHandle(const std::string& schemaType) const = 0;

    // Load and return a renderable document (called on background thread)
    virtual std::shared_ptr<GOW::IDocumentContent>
        load(const ParsedEntry& entry, OpenWad& wad) = 0;
};
