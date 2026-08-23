// `GoWToolkit texture <file> <name> [--out <path.png>]`
//
// Decodes one GOWR texture and reports what came out, optionally writing it
// to a PNG.
//
// Built because the material work kept running into questions the existing
// commands could not answer. `inspect` says which texture won a role;
// `decode` says a texture's dimensions and nothing else. Neither can say
// whether a diffuse carries an alpha channel -- which is exactly what decides
// whether hair renders as strands or as solid shards, since GOWR's beard
// materials declare no separate coverage map and must be masking with the
// diffuse's own alpha, or with nothing at all.
//
// The alpha histogram is the point, not the PNG: a texture whose alpha is 255
// everywhere is opaque by construction and no cutoff will help it, while one
// with a spread is a mask and the only question left is the threshold.

#include "CliApp.h"

#include "core/harness/AssetHarness.h"
#include "core/loaders/GOWRLoaders.h"
#include "core/parsers/gowr/MaterialParser.h"
#include "core/parsers/gowr/TextureDecode.h"

#include <Onyx/Cli/Commands.h>
#include <Onyx/Parsers/SceneNode.h>

// Declarations only. Onyx_CliRender's own Render.cpp already compiles the
// implementation and this executable links it, so defining
// STB_IMAGE_WRITE_IMPLEMENTATION here too is a duplicate-symbol link error --
// which is exactly what it was on the first attempt.
#include <stb_image_write.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace Onyx {

namespace {

// The trailing 16 hex digits of an asset name are its hash -- the same key
// the texpack index uses. Duplicated from MaterialParser's file-local helper
// rather than exported: this command takes a NAME from the command line,
// where the parser takes one out of a material's reference list.
bool HashFromAssetName(const std::string& name, uint64_t& out) {
    if (name.size() < 17 || name[name.size() - 17] != '_') return false;
    uint64_t v = 0;
    for (size_t i = name.size() - 16; i < name.size(); ++i) {
        const char c = name[i];
        uint64_t d;
        if      (c >= '0' && c <= '9') d = (uint64_t)(c - '0');
        else if (c >= 'A' && c <= 'F') d = (uint64_t)(c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') d = (uint64_t)(c - 'a' + 10);
        else return false;
        v = (v << 4) | d;
    }
    out = v;
    return true;
}

void ReportChannels(const Parsers::TextureData& tex, std::ostream& out) {
    const size_t n = tex.pixels.size() / 4;
    if (n == 0) return;

    std::array<size_t, 8> alphaBuckets{};   // 0-31, 32-63, ... 224-255
    size_t opaque = 0, clear = 0;
    uint8_t aMin = 255, aMax = 0;
    for (size_t i = 0; i < n; ++i) {
        const uint8_t a = tex.pixels[i * 4 + 3];
        alphaBuckets[a / 32]++;
        if (a == 255) ++opaque;
        if (a == 0) ++clear;
        aMin = std::min(aMin, a);
        aMax = std::max(aMax, a);
    }

    out << "  alpha: min=" << (int)aMin << " max=" << (int)aMax
        << "  fully opaque " << (100.0 * opaque / n) << "%"
        << "  fully clear " << (100.0 * clear / n) << "%\n";

    if (opaque == n) {
        out << "  -> no alpha channel in use: this texture cannot mask anything.\n";
        return;
    }
    out << "  alpha histogram (32-wide buckets):\n";
    for (size_t b = 0; b < alphaBuckets.size(); ++b) {
        if (!alphaBuckets[b]) continue;
        out << "    " << (b * 32) << "-" << (b * 32 + 31) << "\t"
            << alphaBuckets[b] << "\t(" << (100.0 * alphaBuckets[b] / n) << "%)\n";
    }
}

} // namespace

int CliApp::HandleTexture(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Usage: GoWToolkit texture <wad-file> <texture-name> "
                     "[--out <path.png>]\n"
                     "Example: GoWToolkit texture r_baldur00.wad "
                     "TX_baldur00_beard_d_C5EB108C24574B6A --out beard.png\n";
        return Onyx::Cli::kUsage;
    }

    std::string outPath;
    for (size_t i = 3; i < args.size(); ++i)
        if (args[i] == "--out" && i + 1 < args.size()) outPath = args[++i];

    const std::string& name = args[2];
    uint64_t hash = 0;
    if (!HashFromAssetName(name, hash)) {
        std::cerr << "'" << name << "' does not end in the 16 hex digits that "
                     "name a texture's hash.\n";
        return Onyx::Cli::kUsage;
    }

    Parsers::TextureData tex;
    std::string err;
    if (!GOWRDecodeTexture(GetTexIndex(), hash, name, tex, err)) {
        // The WAD is only needed for the resident fallback, so it is opened
        // lazily -- a texture that streams from a texpack never touches it.
        Onyx::Harness::LoadRequest req{std::filesystem::path(args[1]), "", ""};
        Onyx::Harness::LoadResult  load;
        if (!Onyx::Harness::LoadContainer(req, load)) {
            std::cerr << "texpack: " << err << "; and the WAD would not open: "
                      << load.error << "\n";
            return Onyx::Cli::kUsage;
        }
        std::cerr << "not decodable: " << err << "\n";
        return Onyx::Cli::kUsage;
    }

    std::cout << name << "\n  " << tex.width << "x" << tex.height
              << "  " << tex.pixels.size() << " bytes RGBA8\n";
    ReportChannels(tex, std::cout);

    if (!outPath.empty()) {
        if (!stbi_write_png(outPath.c_str(), (int)tex.width, (int)tex.height, 4,
                            tex.pixels.data(), (int)tex.width * 4)) {
            std::cerr << "could not write " << outPath << "\n";
            return Onyx::Cli::kUsage;
        }
        std::cout << "  wrote " << outPath << "\n";
    }
    return Onyx::Cli::kOk;
}

} // namespace Onyx
