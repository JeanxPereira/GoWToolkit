#pragma once
#include <string>

namespace GOW {

class IDocumentContent {
public:
    virtual ~IDocumentContent() = default;

    virtual std::string GetName() const = 0;
    virtual void Draw() = 0;

    // Opcional: para permitir fechar a aba via código
    virtual bool IsOpen() const { return m_isOpen; }
    virtual void SetOpen(bool open) { m_isOpen = open; }

protected:
    bool m_isOpen = true;
};

} // namespace GOW
