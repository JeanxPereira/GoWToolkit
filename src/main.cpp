#include "window/Window.h"
#include "core/ProfileManager.h"
#include "core/profiles/gow2/ProfileGOW2.h"
#include "core/profiles/gowr/ProfileGOWR.h"
#include "cli/CliApp.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#endif

static void registerProfiles() {
    GOW::ProfileManager::Get().RegisterProfile(std::make_shared<GOW::ProfileGOW2>());
    GOW::ProfileManager::Get().RegisterProfile(std::make_shared<GOW::ProfileGOWR>());
}

int main(int argc, char** argv) {
    registerProfiles();

    if (argc > 1) {
#ifdef _WIN32
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            (void)freopen("CONOUT$", "w", stdout);
            (void)freopen("CONOUT$", "w", stderr);
        }
#endif
        GOW::CliApp::Run(argc, argv);
        return 0;
    }

    Window::initNative();
    Window window;
    window.loop();
    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}
#endif
