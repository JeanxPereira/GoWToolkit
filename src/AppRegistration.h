#pragma once

class App;

namespace Onyx {

// Installs the GoW-specific panel/viewer registrar onto the (engine-generic)
// App. Call once, before App::init() runs, so the registrar is in place when
// App::registerPanels() invokes it. This is APP code — it is the executable's
// job to wire the game UI; the engine ships none of its own.
void InstallGoWPanels(App& app);

} // namespace Onyx
