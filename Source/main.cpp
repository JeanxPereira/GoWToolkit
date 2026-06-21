#include <Onyx/App/Window.h>
#include "AppRegistration.h"
#include <Onyx/Services/ProfileManager.h>
#include "core/profiles/gow2/ProfileGOW2.h"
#include "core/profiles/gowr/ProfileGOWR.h"
#include "core/types/GameTypes.h"
#include "core/profiles/AssetVisibilityDefaults.h"
#include <Onyx/Services/Threading.h>
#include "cli/CliApp.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#endif

static void registerProfiles() {
    Onyx::Services::ProfileManager::Get().RegisterProfile(std::make_shared<Onyx::ProfileGOW2>());
    Onyx::Services::ProfileManager::Get().RegisterProfile(std::make_shared<Onyx::ProfileGOWR>());
}

int main(int argc, char** argv) {
    // Record the main thread before anything else spawns workers.
    Onyx::Threading::MarkMainThread();

    // Populate the asset-type catalog before any parse or UI draw â€” every
    // GameTypes:: handle is invalid until this runs.
    Onyx::GameTypes::RegisterGameTypes();

    // Seed the game-specific default asset visibility table. The store itself
    // holds no game knowledge; this app-level call owns the GoW defaults.
    Onyx::RegisterGameVisibilityDefaults();

    registerProfiles();

    if (argc > 1) {
#ifdef _WIN32
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            (void)freopen("CONOUT$", "w", stdout);
            (void)freopen("CONOUT$", "w", stderr);
        }
#endif
        Onyx::CliApp::Run(argc, argv);
        return 0;
    }

    Onyx::App::Window::initNative();
    Onyx::App::Window window;
    // Inject the game panel/viewer registrar BEFORE the App initializes â€” run()
    // calls App::init(), which invokes the registrar from registerPanels().
    Onyx::InstallGoWPanels(window.app());
    window.run();
    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}
#endif
