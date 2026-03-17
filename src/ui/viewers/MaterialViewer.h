#pragma once

#include "IDocumentContent.h"
#include "core/parsers/gow2/MaterialParser.h"
#include <string>
#include <memory>

namespace GOW {

class MaterialViewer : public IDocumentContent {
public:
    MaterialViewer(const std::string& name, std::unique_ptr<GOW2MaterialParser::MaterialData> matData);
    ~MaterialViewer() override = default;

    std::string GetName() const override;
    void Draw() override;

private:
    std::string m_name;
    std::unique_ptr<GOW2MaterialParser::MaterialData> m_matData;
};

} // namespace GOW
