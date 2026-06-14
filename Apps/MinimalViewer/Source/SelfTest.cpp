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

    // 3) Format a hex dump and validate it is non-trivial.
    std::string dump = FormatHexDump(bytes, /*maxBytes=*/256);
    if (dump.find("0000") == std::string::npos) {
        std::fprintf(stderr, "selftest: hex dump missing offset column\n"); return 5;
    }
    std::printf("selftest OK: %zu bytes, type id=%u\n", bytes.size(), id.value);
    return 0;
}

}
