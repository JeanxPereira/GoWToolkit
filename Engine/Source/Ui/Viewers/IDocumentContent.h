#pragma once
#include <string>



namespace Onyx::Viewers {

class Viewport3D;

class IDocumentContent {
public:
    virtual ~IDocumentContent() = default;

    virtual std::string GetName() const = 0;
    virtual void Draw() = 0;

    // Default implementation does nothing: specific viewers will override this to render robust properties
    virtual void DrawInspector() {}

    // Returns the embedded 3D viewport when the document owns or is one. Used
    // by the Camera panel to bind to the active viewer's camera regardless of
    // whether the active document is a bare Viewport3D (Object viewer) or a
    // composite document like MapViewer that wraps a Viewport3D inside.
    virtual Viewport3D* GetEmbeddedViewport() { return nullptr; }

    // Opcional: para permitir fechar a aba via cÃ³digo
    virtual bool IsOpen() const { return m_isOpen; }
    virtual void SetOpen(bool open) { m_isOpen = open; }

protected:
    bool m_isOpen = true;
};

} // namespace Onyx::Viewers
