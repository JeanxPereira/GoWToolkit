// Test-only stubs for symbols the production parsers reach into but that
// live behind the UI-coupled loader/handler layers, which the test binary
// deliberately does not compile. Anything declared here must match the
// header signatures the parser-min library was built against.
//
// If the production loaders move or gain new external symbols, the link
// step will fail and direct you here.

#include "core/loaders/GOWRLoaders.h"
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

// GowrModule::DecodeMesh calls this. The real implementation lives in
// GOWRLoaders.cpp, which the test binary does not compile: it pulls in
// Viewport3D, ImGui and the whole viewer layer.
//
// Returning null here is honest -- and it means the suite does NOT cover mesh
// decoding. That is the same hole the GUI fell into: while the decoder was a
// stub in production, every mesh click answered "decode failed" and no test
// could have noticed, because no test reaches this path at all. Covering it
// needs a fixture with real MG/MESH geometry plus the loader in the target;
// neither exists yet.
std::unique_ptr<Parsers::SceneData> BuildGowrScene(const Domain::AssetEntry&,
                                                    Domain::AssetContainer&,
                                                    bool, GowrSceneMeta&, int) {
    return nullptr;
}

} // namespace Onyx
