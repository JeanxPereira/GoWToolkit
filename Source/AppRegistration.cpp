#include "AppRegistration.h"

#include <Onyx/App/App.h>
#include "imgui_internal.h"

#include <Onyx/Services/AppConfig.h>
#include <Onyx/Services/Events.h>

// Generic opt-in panels (previously auto-registered by the engine)
#include <Onyx/App/Panels/IsoBrowser.h>
#include <Onyx/App/Panels/PakBrowser.h>
#include <Onyx/App/Panels/CameraPanel.h>
#include <Onyx/App/Panels/AnimCurveView.h>
#include <Onyx/App/Panels/Dopesheet.h>
#include <Onyx/App/Panels/WadStatsView.h>

// Game panels
#include "ui/Inspector.h"
#include "ui/WadBrowser.h"

// Game viewers
#include "ui/viewers/SoundPlayer.h"

// Game modules (Onyx v1.1 IGameModule contract). Replaces the pre-v1.1
// Onyx::Services::ProfileManager::Get().RegisterProfile(...) calls that used
// to live in main.cpp's registerProfiles() -- see task-4-report.md.
#include "core/modules/Gow2Module.h"
#include "core/modules/GowrModule.h"

#include <memory>

namespace Onyx {

void InstallGoWPanels(Onyx::App::App& app) {
  app.SetRegistrar([](Onyx::App::App& a) {
    // Register the game modules with the Workspace. App::AddModule is valid
    // only pre-init ("a call after init() has completed is refused" --
    // App.h), and this registrar runs from inside App::init() (via
    // registerPanels(), before init() returns), which satisfies that
    // window -- the same place the old ProfileManager registration used to
    // run from indirectly (main.cpp, before window.run()).
    a.AddModule(std::make_unique<Onyx::Gowr::GowrModule>());
    a.AddModule(std::make_unique<Onyx::Gow2::Gow2Module>());

    // Generic panels GoWToolkit opts into (previously auto-registered by the
    // engine; now app-composed after Onyx panel-composition change).
    // Default-hidden: ISO Browser, Anim Curves, WAD Stats, Dopesheet.
    // Default-visible: PAK Browser, Camera.
    { auto p = std::make_unique<Onyx::App::IsoBrowser>(); p->visible = false; a.addPanel(std::move(p)); }
    a.addPanel(std::make_unique<Onyx::App::PakBrowser>());
    a.addPanel(std::make_unique<Onyx::App::CameraPanel>());
    { auto p = std::make_unique<Onyx::Viewers::AnimCurveView>(); p->visible = false; a.addPanel(std::move(p)); }
    { auto p = std::make_unique<Onyx::Viewers::WadStatsView>(); p->visible = false; a.addPanel(std::move(p)); }
    { auto p = std::make_unique<Onyx::Viewers::Dopesheet>(); p->visible = false; a.addPanel(std::move(p)); }

    if (auto* cfg = a.getConfig()) cfg->windowTitle = "God Of War Toolkit";

    a.SetDefaultLayout([](ImGuiID dockspace) {
      ImGui::DockBuilderRemoveNode(dockspace);
      ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspace, ImGui::GetMainViewport()->Size);
      ImGuiID dock_main = dockspace;
      ImGuiID dock_left = 0;
      ImGuiID dock_bottom = 0;
      ImGuiID dock_right = 0;
      ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left,  0.22f, &dock_left,   &dock_main);
      ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down,  0.20f, &dock_bottom, &dock_main);
      ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, &dock_right,  &dock_main);
      ImGui::DockBuilderDockWindow("PAK Browser", dock_left);
      ImGui::DockBuilderDockWindow("WAD Browser", dock_left);
      ImGui::DockBuilderDockWindow("Viewer",      dock_main);
      ImGui::DockBuilderDockWindow("Inspector",   dock_right);
      ImGui::DockBuilderDockWindow("Camera",      dock_right); // tab next to Inspector
      ImGui::DockBuilderDockWindow("Log",         dock_bottom);
      ImGui::DockBuilderFinish(dockspace);
    });

    // Game (app) panels
    a.addPanel(std::make_unique<WadBrowser>());
    a.addPanel(std::make_unique<Inspector>());

    // Audio-volume <-> config sync. SoundPlayer (a game viewer) hosts the live
    // volume; App used to bridge it to AppConfig directly. Keep that bridge on
    // the app side via lifecycle events so the engine stays game-agnostic:
    //   • restore config -> SoundPlayer once at startup,
    //   • mirror SoundPlayer -> config every frame so it persists on exit.
    // The registrar runs during App::init() after the config pointer is set,
    // so getConfig() is valid here.
    //
    // The mirror runs on EventFrameEnd (posted AFTER panels/documents draw),
    // not EventFrameTick (posted before the draw). The SoundPlayer volume
    // slider mutates s_volume during the draw, so reading it post-draw captures
    // a same-frame change the same frame — matching the original frameEnd()
    // write-back timing.
    Onyx::Services::AppConfig* config = a.getConfig();
    EventStartupFinished::subscribe([config] {
      if (config)
        Onyx::SoundPlayer::s_volume = config->audioVolume;
    });
    EventFrameEnd::subscribe([config] {
      if (config)
        config->audioVolume = Onyx::SoundPlayer::s_volume;
    });
  });
}

} // namespace Onyx
