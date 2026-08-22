#pragma once

#include <string>

namespace Onyx::App { class App; }

namespace Onyx {

// What `--gui` asked to be opened, if anything. Empty path = plain GUI launch.
//
// Exists because there was no way to reach the GUI with a file from the
// command line at all: main() sent every argc > 1 straight to the CLI, so
// "Open with", a shortcut, a repro script, or any automated check of the
// viewer had to go through the file dialog by hand.
struct GuiStartup {
    std::string path;    // container to open on startup
    std::string entry;   // optional entry name to select once the tree is ready
};

// Installs the GoW-specific panel/viewer registrar onto the (engine-generic)
// App. Call once, before App::init() runs, so the registrar is in place when
// App::registerPanels() invokes it. This is APP code — it is the executable's
// job to wire the game UI; the engine ships none of its own.
void InstallGoWPanels(Onyx::App::App& app, const GuiStartup& startup = {});

} // namespace Onyx
