// Test-only stub for GOW::Platform::DetectSystemAppearance().
// The test binary does not compile the platform-specific .mm / .cpp so we
// provide a trivial fallback. Theme tests call ApplyTheme with an explicit
// mode, so this is only reached if ThemeMode::System leaks in.

#include "core/platform/SystemTheme.h"

namespace GOW::Platform {

SystemAppearance DetectSystemAppearance() {
    return SystemAppearance::Dark;   // safe fallback for test builds
}

} // namespace GOW::Platform
