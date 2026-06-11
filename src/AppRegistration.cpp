#include "AppRegistration.h"

#include "App.h"

#include "core/AppConfig.h"
#include "core/Events.h"

// Game panels
#include "ui/Inspector.h"
#include "ui/WadBrowser.h"

// Game viewers
#include "ui/viewers/SoundPlayer.h"

#include <memory>

namespace Onyx {

void InstallGoWPanels(App& app) {
  app.SetRegistrar([](App& a) {
    // Game (app) panels — these were previously hardcoded in
    // App::registerPanels(). Constructor args match verbatim.
    a.addPanel(std::make_unique<WadBrowser>());
    a.addPanel(std::make_unique<Inspector>());

    // Audio-volume <-> config sync. SoundPlayer (a game viewer) hosts the live
    // volume; App used to bridge it to AppConfig directly. Keep that bridge on
    // the app side via lifecycle events so the engine stays game-agnostic:
    //   • restore config -> SoundPlayer once at startup,
    //   • mirror SoundPlayer -> config every frame so it persists on exit.
    // The registrar runs during App::init() after the config pointer is set,
    // so getConfig() is valid here.
    AppConfig* config = a.getConfig();
    EventStartupFinished::subscribe([config] {
      if (config)
        Onyx::SoundPlayer::s_volume = config->audioVolume;
    });
    EventFrameTick::subscribe([config] {
      if (config)
        config->audioVolume = Onyx::SoundPlayer::s_volume;
    });
  });
}

} // namespace Onyx
