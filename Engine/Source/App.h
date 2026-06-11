#pragma once
#include "imgui.h"
#include "Ui/PanelRegistry.h"

#include "Ui/WindowDecorator.h"
#include "Ui/Viewers/DocumentWindow.h"
#include "Ui/ViewerRegistry.h"
#include "Core/AssetDatabase.h"
#include "Core/AppConfig.h"
#include "Core/RecentFiles.h"
#include <GLFW/glfw3.h>
#include <functional>
#include <memory>
#include <string>

class App {
public:
    App();
    void init(GLFWwindow* window, AppConfig* config);

    // Hook the executable uses to register game-specific panels and viewers.
    // Set before init(); invoked once during init, after the engine's generic
    // panels are registered. Keeps App game-agnostic â€” the engine ships no
    // game wiring of its own.
    using AppRegistrar = std::function<void(App&)>;
    void SetRegistrar(AppRegistrar r) { m_registrar = std::move(r); }

    // Minimal accessor the registrar uses to add a (game) panel.
    void addPanel(std::unique_ptr<IPanel> panel) { m_panels.add(std::move(panel)); }

    // Frame phases â€” called by Window
    void frameBegin();
    void frame();
    void frameEnd();

    bool wantClose() const { return m_wantClose; }

    // UI Component Getters (for external access)
    AssetDatabase& getDatabase() { return m_db; }
    AppConfig*     getConfig() { return m_config; }
    Onyx::DocumentWindow& getDocumentWindow() { return m_documentWindow; }
    Onyx::ViewerRegistry& getViewerRegistry() { return m_viewerRegistry; }

private:
    void drawMenuBar();
    void drawMenuItems();
    void drawPopups();
    void drawOpenDialog();
    void setupDockLayout(ImGuiID dockspace_id);
    void registerPanels();
    void openRecentFile(RecentEntry entry);
    std::string getRecentsPath() const;

    AssetDatabase         m_db;
    PanelRegistry         m_panels;
    Onyx::DocumentWindow   m_documentWindow;
    Onyx::ViewerRegistry   m_viewerRegistry;
    WindowDecorator       m_decorator;
    AppConfig*            m_config  = nullptr;
    GLFWwindow*           m_window  = nullptr;
    bool                  m_wantClose         = false;
    bool                  m_layoutInitialized = false;

    // Injected game-specific panel/viewer registrar (supplied by the app).
    AppRegistrar          m_registrar;

    // Recents
    RecentFiles           m_recentFiles;

    // Open dialog state
    bool m_showOpenDialog = false;
    int  m_openDialogSelectedGame = 1; // Default: GOW2
};
