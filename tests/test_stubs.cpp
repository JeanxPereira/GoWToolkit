// Test-only stubs for symbols the production parsers reach into but that
// live behind the UI-coupled loader/handler layers, which the test binary
// deliberately does not compile. Anything declared here must match the
// header signatures the parser-min library was built against.
//
// If the production loaders move or gain new external symbols, the link
// step will fail and direct you here.

#include "core/parsers/gowr/TexPackIndex.h"
#include <filesystem>

namespace Onyx {

TexPackIndex& GetTexIndex() {
    // No real index — golden tests only need ParseContainer to terminate, not to
    // resolve textures from .texpack archives.
    static TexPackIndex stub;
    return stub;
}

// ProfileGOWR::PrepareForParse calls these before ParseContainer. In tests we
// have no game root to detect, so both are no-ops (EnsureGowrConfigIni returns
// false so InvalidateLodIndex is never called, but stubs are needed to link).
bool EnsureGowrConfigIni(const std::filesystem::path&) { return false; }
void InvalidateLodIndex() {}

} // namespace Onyx
