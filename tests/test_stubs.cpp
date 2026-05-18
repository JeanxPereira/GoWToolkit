// Test-only stubs for symbols the production parsers reach into but that
// live behind the UI-coupled loader/handler layers, which the test binary
// deliberately does not compile. Anything declared here must match the
// header signatures the parser-min library was built against.
//
// If the production loaders move or gain new external symbols, the link
// step will fail and direct you here.

#include "core/parsers/gowr/TexPackIndex.h"

namespace GOW {

TexPackIndex& GetTexIndex() {
    // No real index — golden tests only need ParseWad to terminate, not to
    // resolve textures from .texpack archives.
    static TexPackIndex stub;
    return stub;
}

} // namespace GOW
