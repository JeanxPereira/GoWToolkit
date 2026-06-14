#include "SelfTest.h"
#include "HexViewer.h"

#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Domain/MediaKind.h>
#include <Onyx/Vfs/OsFile.h>

#include <cstdio>
#include <vector>
#include <cstdint>

namespace MinimalViewer {

int RunSelfTest(const char* path) {
    // 1) Register a toy type purely through the public catalog API.
    Onyx::Types::TypeInfo info;
    info.key   = "MINIMAL_RAW_BLOCK";
    info.label = "Raw Binary Block";
    info.media = Onyx::Domain::MediaKind::Raw;
    info.icon  = nullptr;
    Onyx::Types::TypeId id = Onyx::Types::TypeCatalog::Get().Register(info);
    if (!id.valid()) { std::fprintf(stderr, "selftest: type registration failed\n"); return 1; }
    if (Onyx::Types::TypeCatalog::Get().Media(id) != Onyx::Domain::MediaKind::Raw) {
        std::fprintf(stderr, "selftest: media routing wrong\n"); return 2;
    }

    // 2) Open the file via the public VFS and read all bytes.
    Onyx::Vfs::OsFile file(path);
    if (!file.IsValid()) { std::fprintf(stderr, "selftest: cannot open %s\n", path); return 3; }
    std::vector<uint8_t> bytes = file.ReadAll();
    if (bytes.empty()) { std::fprintf(stderr, "selftest: empty read\n"); return 4; }

    // 3) Validate the hex formatter deterministically against a known input:
    //    offset column, byte column (uppercase hex), and ASCII gutter.
    const std::vector<uint8_t> probe = {0x00, 0x41, 0xFF}; // NUL, 'A', 0xFF -> ".A."
    const std::string probeDump = FormatHexDump(probe, probe.size());
    if (probeDump.substr(0, 8) != "00000000" ||
        probeDump.find("00 41 FF") == std::string::npos ||
        probeDump.find(".A.") == std::string::npos) {
        std::fprintf(stderr, "selftest: hex formatter output wrong:\n%s\n", probeDump.c_str());
        return 5;
    }

    // 4) Exercise the real path on the opened file's bytes.
    if (FormatHexDump(bytes, /*maxBytes=*/256).empty()) {
        std::fprintf(stderr, "selftest: empty dump from file bytes\n"); return 6;
    }
    std::printf("selftest OK: %zu bytes, type id=%u\n", bytes.size(), id.value);
    return 0;
}

}
