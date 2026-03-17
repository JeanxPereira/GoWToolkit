#pragma once
#include "imgui.h"
#include "ui/PanelRegistry.h"
#include "ui/AppContext.h"
#include "ui/WindowDecorator.h"
#include "ui/viewers/DocumentWindow.h"
#include "ui/ViewerRegistry.h"
#include "core/AssetDatabase.h"
#include "core/AppConfig.h"
#include <GLFW/glfw3.h>
#include <memory>
#include <string>

class App {
public:
    App();
    void init(GLFWwindow* window, AppConfig* config);

    // Frame phases — called by Window
    void frameBegin();
    void frame();
    void frameEnd();

    bool wantClose() const { return m_wantClose; }

    // UI Component Getters (for external access)
    AssetDatabase& getDatabase() { return m_db; }
    GOW::DocumentWindow& getDocumentWindow() { return m_documentWindow; }
    GOW::ViewerRegistry& getViewerRegistry() { return m_viewerRegistry; }

private:
    void drawMenuBar();
    void drawMenuItems();
    void drawPopups();
    void drawOpenDialog();
    void setupDockLayout(ImGuiID dockspace_id);
    void registerPanels();
    void registerViewers();

    AssetDatabase         m_db;
    PanelRegistry         m_panels;
    GOW::DocumentWindow   m_documentWindow;
    GOW::ViewerRegistry   m_viewerRegistry;
    WindowDecorator       m_decorator;
    AppConfig*            m_config  = nullptr;
    GLFWwindow*           m_window  = nullptr;
    ParsedEntry*          m_selected = nullptr;
    bool                  m_wantClose         = false;
    bool                  m_layoutInitialized = false;

    // Open dialog state
    bool m_showOpenDialog = false;
    int  m_openDialogSelectedGame = 1; // Default: GOW2
};
