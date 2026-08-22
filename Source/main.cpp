#include <Onyx/App/Window.h>
#include "AppRegistration.h"
#include "core/types/GameTypes.h"
#include "core/profiles/AssetVisibilityDefaults.h"
#include <Onyx/Services/Threading.h>
#include "cli/CliApp.h"
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#endif

int main(int argc, char** argv) {
    // Record the main thread before anything else spawns workers.
    Onyx::Threading::MarkMainThread();

    // Populates the legacy Onyx::GameTypes:: catalog. This is now UNRELATED
    // to how Gow2Module/GowrModule stamp a parsed tree's AssetEntry::typeId
    // (Phase 2 Task 4 converged both onto their own TypeRegistrar-minted
    // handles, registered when InstallGoWPanels' registrar calls
    // App::AddModule -- see AppRegistration.cpp). It is still required,
    // separately, by the older per-type Onyx::Types::ITypeHandler system
    // (Source/core/types/handlers/*.cpp, self-registered via
    // ONYX_REGISTER_FILE_TYPE/REGISTER_GOW_TYPE/REGISTER_GOW_TAG) that
    // AssetHarness::Load()'s BuildSceneData dispatch and the GUI's
    // viewer-creation path still read through -- those handlers' GetId()
    // stays an invalid/zero handle until this runs. Retiring that system
    // onto the module/DecoderRegistry contract is out of this phase's
    // scope (Phase 3/5); until then, both catalogs must be populated.
    Onyx::GameTypes::RegisterGameTypes();

    // Seed the game-specific default asset visibility table. The store itself
    // holds no game knowledge; this app-level call owns the GoW defaults.
    Onyx::RegisterGameVisibilityDefaults();

    // `--gui <file> [entry]` launches the window on a container instead of
    // dispatching to the CLI. Without it there is no way to reach the viewer
    // from a command line -- every argc > 1 went to CliApp -- which blocks
    // "Open with", shortcuts, repro scripts, and any automated check of the
    // GUI itself.
    Onyx::GuiStartup startup;
    int firstArg = 1;
    if (argc > 1 && std::string(argv[1]) == "--gui") {
        if (argc > 2) startup.path  = argv[2];
        if (argc > 3) startup.entry = argv[3];
        firstArg = argc;   // nothing left for the CLI
    }

    if (argc > 1 && firstArg < argc) {
#ifdef _WIN32
        // A Release build is a GUI-subsystem binary, so it starts with no
        // console and CLI output would go nowhere. Attaching the parent's
        // console fixes that -- but ONLY for a stream that is not already
        // going somewhere: reopening CONOUT$ unconditionally overwrites a
        // caller's redirection, so `GoWToolkit list x.wad > out.txt` and every
        // pipe silently produced an empty file. A stream the shell redirected
        // arrives with a valid std handle already; one that did not arrives
        // null, and only that one gets pointed at the console.
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            if (GetStdHandle(STD_OUTPUT_HANDLE) == nullptr)
                (void)freopen("CONOUT$", "w", stdout);
            if (GetStdHandle(STD_ERROR_HANDLE) == nullptr)
                (void)freopen("CONOUT$", "w", stderr);
        }
#endif
        // Propagate the CLI's status: scripts and the headless render harness
        // need a non-zero exit to mean failure.
        return Onyx::CliApp::Run(argc, argv);
    }

    Onyx::App::Window::initNative();
    Onyx::App::Window window;
    // Inject the game panel/viewer registrar BEFORE the App initializes — run()
    // calls App::init(), which invokes the registrar from registerPanels().
    Onyx::InstallGoWPanels(window.app(), startup);
    window.run();
    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}
#endif
