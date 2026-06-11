#pragma once

namespace Onyx::Platform {

// Resolved at the moment of the call (no observer / no caching) — caller
// stores the result. Returns Dark on platforms or fallbacks where the OS
// preference can't be read so the UI never lands on an unreadable Light base.
enum class SystemAppearance { Dark, Light };

SystemAppearance DetectSystemAppearance();

} // namespace Onyx::Platform
