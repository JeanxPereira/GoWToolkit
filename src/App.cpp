#define IMGUI_DEFINE_MATH_OPERATORS
#include "App.h"
#include "UIHelpers.h"
#include "imgui_internal.h" // DockBuilder

#include "ui/IPanel.h"
#include "ui/NativeMenuBar.h"
#include "ui/NativeWindow.h"
#include "ui/TitleBar.h"

#include "ui/CameraPanel.h"
#include "ui/Inspector.h"
#include "ui/IsoBrowser.h"
#include "ui/PakBrowser.h"
#include "ui/SettingsWindow.h"
#include "ui/StatusBar.h"
#include "ui/WadBrowser.h"

// Viewer headers
#include "ui/viewers/ImageViewer.h"
#include "ui/viewers/MaterialViewer.h"
#include "ui/viewers/SoundPlayer.h"
#include "ui/viewers/Viewport3D.h"
#include "ui/viewers/AnimCurveView.h"
#include "ui/viewers/Dopesheet.h"
#include "ui/viewers/WadStatsView.h"

// Core subsystems
#include "core/Events.h"
#include "core/TaskManager.h"
#include "core/ToolkitApi.h"

#include "core/Logger.h"
#include "core/PathUtils.h"
#include "core/FontManager.h"
#include "fonts/SFSymbols.h"

void App::registerPanels() {
  m_panels.add(std::make_unique<IsoBrowser>());
  m_panels.add(std::make_unique<PakBrowser>());
  m_panels.add(std::make_unique<WadBrowser>());
  m_panels.add(std::make_unique<Inspector>());
  m_panels.add(std::make_unique<CameraPanel>());
  m_panels.add(std::make_unique<StatusBar>());
  m_panels.add(std::make_unique<SettingsWindow>());
  m_panels.add(std::make_unique<AnimCurveView>());
  m_panels.add(std::make_unique<Dopesheet>());
  m_panels.add(std::make_unique<WadStatsView>());

  // Set initial visibility
  if (auto *iso = dynamic_cast<IsoBrowser *>(m_panels.find("ISO Browser")))
    iso->visible = false;
  if (auto *settings =
          dynamic_cast<SettingsWindow *>(m_panels.find("Settings")))
    settings->visible = false;
  if (auto *animCurves =
          dynamic_cast<AnimCurveView *>(m_panels.find("Anim Curves")))
    animCurves->visible = false;
  if (auto *wadStats =
          dynamic_cast<WadStatsView *>(m_panels.find("WAD Stats")))
    wadStats->visible = false;
  if (auto *dope = dynamic_cast<Dopesheet *>(m_panels.find("Dopesheet")))
    dope->visible = false;
}

App::App() {}

void App::init(GLFWwindow *window, AppConfig *config) {
  m_window = window;
  m_config = config;

  // Initialize core subsystems
  GOW::Api::InitParams params;
  params.db = &m_db;
  params.config = config;
  params.viewers = &m_viewerRegistry;
  params.documents = &m_documentWindow;
  GOW::Api::Init(params);

  // Initialize centralized font system
  GOW::Fonts::Init();
  GOW::Fonts::SetIconFont({
      PathUtils::resolvePath("third_party/fonts/SFSymbols.ttf"),
      0xE000, 0xFA19,
      {0.0f, 3.0f}
  });

  // On macOS: nativeDecorations=true means use traffic lights
  // (borderless=false) On Windows/Linux: always borderless custom titlebar
#if defined(__APPLE__)
  m_decorator.borderless = !config->nativeDecorations;
#else
  m_decorator.borderless = true;
#endif

  // WindowDecorator init — icon font is now managed by FontManager
  m_decorator.init(window, nullptr);

  // Restore persisted audio volume
  GOW::SoundPlayer::s_volume = config->audioVolume;

  // Initialize panels that need config
  registerPanels();
  if (auto *settings =
          dynamic_cast<SettingsWindow *>(m_panels.find("Settings"))) {
    settings->config = config;
    settings->Init();
  }

  // Load recent files
  m_recentFiles.Load(getRecentsPath());

  // Signal startup complete
  EventStartupFinished::post();
}

// ── Frame Phases ────────────────────────────────────────────────────────────

void App::frameBegin() {
  // Pass menubar height to native window for NCHITTEST
  if (m_window)
    m_decorator.beginFrame(m_window);
}

void App::frame() {
  // Process task manager deferred calls and garbage collection
  GOW::TaskManager::runDeferredCalls();
  GOW::TaskManager::collectGarbage();


  // Per-frame tick event for animations, progress bars, etc.
  EventFrameTick::post();

  // ── Host window fullscreen ─────────────────────────────────────────────
  ImGuiViewport *vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);
  ImGui::SetNextWindowViewport(vp->ID);

  ImGuiWindowFlags host_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
      ImGuiWindowFlags_MenuBar;

  ImVec2 framePadding = ImGui::GetStyle().FramePadding;
#if defined(GOWTOOL_OS_MACOS)
  if (!NativeWindow::macosIsFullScreen(m_window) && m_decorator.borderless) {
    framePadding.y = 8.0f; // 1:1 ImHex: 8_scaled for macOS titlebar
  }
#endif

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePadding);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::Begin("##HostWindow", nullptr, host_flags);
  ImGui::PopStyleVar(2);

  // ── Global Keyboard Shortcuts ──────────────────────────────────────────
  ImGuiIO &io = ImGui::GetIO();
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
    m_showOpenDialog = true;
  }
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W, false)) {
    m_documentWindow.CloseActiveTab();
  }

  // Decorator: beginFrame needs to happen after Begin("##HostWindow")
  // so it can read the menu bar height
  frameBegin();

  drawMenuBar();

  ImGui::PopStyleVar(); // Pop FramePadding

  // ── DockSpace ──────────────────────────────────────────────────────────
  ImGuiID dockspace_id = ImGui::GetID("GoWToolDockSpace");

  if (!m_layoutInitialized) {
    m_layoutInitialized = true;
    setupDockLayout(dockspace_id);
  }

  ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_None);
  ImGui::End();

  // ── Draw all registered panels ─────────────────────────────────────────
  // ── Draw all registered panels ─────────────────────────────────────────
  m_panels.DrawAll();

  // ── Document Window (tab host) ─────────────────────────────────────────
  m_documentWindow.Draw();

  // ── All popups / modals ────────────────────────────────────────────────
  drawPopups();
}

void App::frameEnd() {
  // Font rebuild is now handled by Window::frameEnd() via GOW::Fonts

  // Sync audio volume back to config so it's saved on exit
  if (m_config)
    m_config->audioVolume = GOW::SoundPlayer::s_volume;
}

void App::setupDockLayout(ImGuiID dockspace_id) {
  ImGui::DockBuilderRemoveNode(dockspace_id);
  ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

  ImGuiID dock_main = dockspace_id;
  ImGuiID dock_left = 0;
  ImGuiID dock_bottom = 0;
  ImGuiID dock_right = 0;

  ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.22f, &dock_left,
                              &dock_main);
  ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.20f, &dock_bottom,
                              &dock_main);
  ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, &dock_right,
                              &dock_main);

  ImGui::DockBuilderDockWindow("PAK Browser", dock_left);
  ImGui::DockBuilderDockWindow("WAD Browser", dock_left);
  ImGui::DockBuilderDockWindow("Viewer", dock_main);
  ImGui::DockBuilderDockWindow("Inspector", dock_right);
  ImGui::DockBuilderDockWindow("Camera", dock_right); // tab next to Inspector
  ImGui::DockBuilderDockWindow("Log", dock_bottom);

  ImGui::DockBuilderFinish(dockspace_id);
}

void App::drawOpenDialog() {
  if (m_showOpenDialog) {
    if (m_db.IsLoading()) {
      // Do not open if already loading
      m_showOpenDialog = false;
    } else {
      ImGui::OpenPopup("Open Game File");
      m_showOpenDialog = false;
    }
  }

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_Appearing);

  if (ImGui::BeginPopupModal("Open Game File", nullptr,
                             ImGuiWindowFlags_NoResize)) {
    ImGui::Spacing();
    ImGui::Text("Select the game version:");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const char *gameVersions[] = {"God of War 1", "God of War 2",
                                  "God of War (2018)", "God of War Ragnarok"};

    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##gameversion", &m_openDialogSelectedGame, gameVersions,
                 IM_ARRAYSIZE(gameVersions));

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::TextDisabled("Supported formats:");
    switch (m_openDialogSelectedGame) {
    case 0: // GOW1
    case 1: // GOW2
      ImGui::BulletText("ISO files (.iso) - Full game image");
      ImGui::BulletText("WAD files (.wad) - Individual resource packs");
      break;
    case 2: // GOW2018
    case 3: // Ragnarok
      ImGui::BulletText("WAD files (.wad) - Resource packs");
      break;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float buttonWidth = 120.0f;
    float totalWidth = buttonWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

    if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
      std::string path = SystemOpenFileDialog();
      if (!path.empty()) {
        fs::path p(path);
        auto ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        switch (m_openDialogSelectedGame) {
        case 0: // GOW1
        case 1: // GOW2
          if (ext == ".iso") {
            m_db.LoadIsoPakAsync(p);
            if (auto *pak =
                    dynamic_cast<PakBrowser *>(m_panels.find("PAK Browser")))
              pak->visible = true;
            ImGui::SetWindowFocus("PAK Browser");
            m_recentFiles.Add(
                path, m_openDialogSelectedGame == 0 ? "gow1" : "gow2", "ISO");
          } else {
            std::string hint = m_openDialogSelectedGame == 0 ? "gow1" : "gow2";
            m_db.LoadWadAsync(p, hint);
            if (auto *wad =
                    dynamic_cast<WadBrowser *>(m_panels.find("WAD Browser")))
              wad->visible = true;
            ImGui::SetWindowFocus("WAD Browser");
            m_recentFiles.Add(path, hint, "WAD");
          }
          break;
        case 2: // GOW2018
        case 3: // Ragnarok
          m_db.LoadWadAsync(p, "ragnarok");
          if (auto *wad =
                  dynamic_cast<WadBrowser *>(m_panels.find("WAD Browser")))
            wad->visible = true;
          ImGui::SetWindowFocus("WAD Browser");
          m_recentFiles.Add(path, "ragnarok", "WAD");
          break;
        }
      }
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void App::openRecentFile(RecentEntry entry) {
  fs::path p(entry.path);
  if (!fs::exists(p))
    return;

  if (entry.fileType == "ISO") {
    m_db.LoadIsoPakAsync(p);
    if (auto *pak = dynamic_cast<PakBrowser *>(m_panels.find("PAK Browser")))
      pak->visible = true;
    ImGui::SetWindowFocus("PAK Browser");
  } else {
    m_db.LoadWadAsync(p, entry.gameHint);
    if (auto *wad = dynamic_cast<WadBrowser *>(m_panels.find("WAD Browser")))
      wad->visible = true;
    ImGui::SetWindowFocus("WAD Browser");
  }

  // Re-add to bump it to the top
  m_recentFiles.Add(entry.path, entry.gameHint, entry.fileType);
}

std::string App::getRecentsPath() const {
  return PathUtils::resolvePath("gowtool_recents.txt");
}

void App::drawPopups() { drawOpenDialog(); }

void App::drawMenuBar() {
  if (!ImGui::BeginMenuBar())
    return;

  // ── Phase 1: Backdrop first so menu items render on top of it ─────────
  if (m_window)
    TitleBar::drawBackDrop();

  // ── Phase 2: Menu items ─────────────────────────────────────────────
  // On macOS with native decorations, menus go to the system menu bar.
  // On Windows/Linux (or macOS borderless), menus render in ImGui.
  // NativeMenuBar::enable() persists across frames — only call it to
  // set state, never toggle on/off per frame (which would clear NSMenu).

#if defined(GOWTOOL_OS_MACOS)
  bool useNativeMenu = m_config && m_config->nativeMenuBar;
  NativeMenuBar::enable(useNativeMenu);
  if (useNativeMenu)
    NativeMenuBar::beginMainMenuBar();
#endif

  drawMenuItems();

#if defined(GOWTOOL_OS_MACOS)
  if (useNativeMenu)
    NativeMenuBar::endMainMenuBar();
#endif

  // ── Phase 3: Titlebar buttons + centered title ──────────────────────
  if (m_window)
    m_wantClose =
        TitleBar::draw(m_window, "God Of War Toolkit", m_decorator.borderless);

  // ── Phase 4: macOS borderless drag ──────────────────────────────────
#if defined(GOWTOOL_OS_MACOS)
  if (m_decorator.borderless) {
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float menuBarH = ImGui::GetCurrentWindowRead()->MenuBarHeight;
    const ImVec2 menuUnderlaySize(windowSize.x, menuBarH);

    ImGui::SetCursorPos(ImVec2());

    if (!ImGui::IsAnyItemHovered()) {
      const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
      if (ImGui::IsMouseHoveringRect(cursorPos, cursorPos + menuUnderlaySize) &&
          ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        NativeWindow::macosHandleTitlebarDoubleClick(m_window);
      }
      NativeWindow::macosSetWindowMovable(m_window, true);
    } else {
      NativeWindow::macosSetWindowMovable(m_window, false);
    }
  }
#endif

  ImGui::EndMenuBar();
}

void App::drawMenuItems() {
  // When NativeMenuBar is enabled (macOS native), these calls populate NSMenu.
  // When disabled, they call ImGui::BeginMenu/MenuItem directly.

#if defined(GOWTOOL_OS_MACOS)
  // Push menu items right of the traffic lights when rendering in ImGui
  if (!NativeMenuBar::isEnabled() && !NativeWindow::macosIsFullScreen(m_window))
    ImGui::SetCursorPosX(80.0f);
#endif

  if (NativeMenuBar::beginMenu("File")) {
    if (NativeMenuBar::menuItem("Open...", "Ctrl+O"))
      m_showOpenDialog = true;
    if (NativeMenuBar::menuItem("Close All"))
      m_db.CloseAll();

    NativeMenuBar::separator();

    // ── Recents submenu ─────────────────────────────────────────────
    if (NativeMenuBar::beginMenu("Recent Files", !m_recentFiles.Empty())) {
      RecentEntry entryToOpen;
      bool shouldOpen = false;

      for (const auto &entry : m_recentFiles.Entries()) {
        // Build label: "filename.iso  [GOW2 · ISO]"
        std::string gameLabel;
        if (entry.gameHint == "gow1")
          gameLabel = "GOW1";
        else if (entry.gameHint == "gow2")
          gameLabel = "GOW2";
        else if (entry.gameHint == "ragnarok")
          gameLabel = "GOWR";
        else
          gameLabel = entry.gameHint;

        // std::string label = entry.displayName + "  [" + gameLabel + " " +
        //                     ICON_SF_CUBE_FILL + " " + entry.fileType + "]";
        std::string label = entry.displayName + " [" + gameLabel + "]";

        if (NativeMenuBar::menuItem(label.c_str())) {
          entryToOpen = entry;
          shouldOpen = true;
        }
      }

      NativeMenuBar::separator();
      if (NativeMenuBar::menuItem("Clear Recents")) {
        m_recentFiles.Clear();
        m_recentFiles.Save(getRecentsPath());
      }
      NativeMenuBar::endMenu();

      if (shouldOpen) {
        openRecentFile(entryToOpen);
      }
    }

    NativeMenuBar::separator();
    if (NativeMenuBar::menuItem("Exit", "Alt+F4"))
      exit(0);
    NativeMenuBar::endMenu();
  }

  if (NativeMenuBar::beginMenu("Export")) {
    bool has = GOW::Api::GetSelected() != nullptr;
    if (NativeMenuBar::menuItem("Export glTF...", "Ctrl+E", false, has)) {
    }
    if (NativeMenuBar::menuItem("Export DDS...", nullptr, false, has)) {
    }
    if (NativeMenuBar::menuItem("Copy Hash", "Ctrl+C", false, has))
      if (GOW::Api::GetSelected())
        ImGui::SetClipboardText(HashHex(GOW::Api::GetSelected()->hash).c_str());
    NativeMenuBar::endMenu();
  }

  if (NativeMenuBar::beginMenu("Options")) {
    if (NativeMenuBar::menuItem("Settings...", "Ctrl+,")) {
      if (auto *settings =
              dynamic_cast<SettingsWindow *>(m_panels.find("Settings")))
        settings->visible = true;
    }
    NativeMenuBar::endMenu();
  }

  if (NativeMenuBar::beginMenu("View")) {
    for (auto &p : m_panels) {
      NativeMenuBar::menuItemToggle(std::string(p->getName()).c_str(), nullptr,
                                    &p->visible);
    }
    if (NativeMenuBar::menuItem("Reset Layout"))
      m_layoutInitialized = false;
    NativeMenuBar::endMenu();
  }
}
