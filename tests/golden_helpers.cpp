#include "golden_helpers.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

#include "core/modules/Gow2Module.h"
#include "core/modules/GowrModule.h"
#include <Onyx/Modules/Workspace.h>
#include <Onyx/Types/TypeId.h>
#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Vfs/MemoryFile.h>
#include <Onyx/Vfs/OsFile.h>

#define XXH_INLINE_ALL
#include "xxhash.h"

namespace fs = std::filesystem;
using nlohmann::ordered_json;

namespace gowtoolkit::testing {

namespace {

// Cap how many payload bytes feed the hash; entries can be megabytes long
// and the goal is regression detection, not cryptographic uniqueness.
constexpr size_t kPayloadHashLimit = 64 * 1024;

std::string ToHex64(uint64_t value) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
    return oss.str();
}

// Reads up to `kPayloadHashLimit` bytes at `offset` from `source` and returns
// xxhash64 of the slice (0 when the entry has no payload).
uint64_t HashEntryPayload(Onyx::Vfs::IFile& source, uint64_t offset, uint64_t size) {
    if (size == 0) return 0;
    size_t toRead = static_cast<size_t>(std::min<uint64_t>(size, kPayloadHashLimit));
    std::vector<uint8_t> buf(toRead);
    source.Seek(static_cast<int64_t>(offset), SEEK_SET);
    size_t got = source.Read(buf.data(), toRead);
    if (got != toRead) buf.resize(got);
    return XXH64(buf.data(), buf.size(), /*seed=*/0);
}

// Hashes through doc.fileTable[entry.source.fileIndex], not a single
// container-wide source: Onyx v1.1 modules can address any file table slot
// (GowrModule's LZ4-decompressed buffer, Gow2Module's ISO PAK slices), and
// getting this wrong would silently hash the wrong bytes -- exactly the
// class of bug FinalizeEntries' range-salvage checks (GowrModule.cpp,
// Gow2Module.cpp) exist to catch on the production side.
void FlattenEntry(const AssetEntry& entry,
                  const std::vector<std::shared_ptr<Onyx::Vfs::IFile>>& fileTable,
                  std::vector<ordered_json>& out) {
    ordered_json e;
    e["name"]        = entry.name;
    e["typeId"]      = Onyx::Types::TypeCatalog::Get().Label(entry.typeId);
    e["size"]        = entry.source.size;
    e["offset"]      = entry.source.offset;
    e["childCount"]  = static_cast<uint64_t>(entry.children.size());
    e["kind"]        = std::string(Onyx::Domain::Name(entry.kind));

    Onyx::Vfs::IFile* source = nullptr;
    if (entry.source.fileIndex < fileTable.size()) {
        source = fileTable[entry.source.fileIndex].get();
    }
    if (source && entry.source.size > 0) {
        e["payloadHash"] = ToHex64(HashEntryPayload(*source, entry.source.offset, entry.source.size));
    } else {
        e["payloadHash"] = "0x0000000000000000";
    }
    out.push_back(std::move(e));

    for (const auto& child : entry.children) {
        FlattenEntry(child, fileTable, out);
    }
}

} // anonymous namespace

ordered_json SnapshotEntries(const Onyx::Modules::Document& doc) {
    std::vector<ordered_json> flat;
    for (const auto& entry : doc.roots) {
        FlattenEntry(entry, doc.fileTable, flat);
    }

    std::sort(flat.begin(), flat.end(), [](const ordered_json& a, const ordered_json& b) {
        uint64_t ao = a.value("offset", uint64_t{0});
        uint64_t bo = b.value("offset", uint64_t{0});
        if (ao != bo) return ao < bo;
        // Tie-break by name so equal-offset entries (rare but possible for
        // 0-sized reference entries) still sort deterministically.
        return a.value("name", std::string{}) < b.value("name", std::string{});
    });

    ordered_json out;
    out["wad"]        = doc.path.filename().string();
    out["entryCount"] = static_cast<uint64_t>(flat.size());
    out["entries"]    = std::move(flat);
    return out;
}

ordered_json LoadGolden(const fs::path& path) {
    std::ifstream in(path);
    if (!in) return nullptr;
    ordered_json doc;
    in >> doc;
    return doc;
}

std::string DiffSnapshots(const ordered_json& actual, const ordered_json& expected) {
    std::ostringstream diff;

    auto note = [&](const std::string& prefix, const std::string& msg) {
        diff << prefix << ": " << msg << '\n';
    };

    if (actual.value("wad", std::string{}) != expected.value("wad", std::string{})) {
        note("[wad]", "actual='" + actual.value("wad", std::string{}) +
                     "' expected='" + expected.value("wad", std::string{}) + "'");
    }

    uint64_t actualCount   = actual.value("entryCount", uint64_t{0});
    uint64_t expectedCount = expected.value("entryCount", uint64_t{0});
    if (actualCount != expectedCount) {
        note("[entryCount]",
             "actual=" + std::to_string(actualCount) +
             " expected=" + std::to_string(expectedCount));
    }

    const auto& aEntries = actual.contains("entries")   ? actual["entries"]   : ordered_json::array();
    const auto& eEntries = expected.contains("entries") ? expected["entries"] : ordered_json::array();
    size_t pairCount = std::min(aEntries.size(), eEntries.size());

    static constexpr const char* kFields[] = {
        "name", "typeId", "size", "offset", "childCount", "kind", "payloadHash"
    };

    for (size_t i = 0; i < pairCount; ++i) {
        const auto& a = aEntries[i];
        const auto& e = eEntries[i];
        for (const char* field : kFields) {
            if (a.value(field, ordered_json{}) != e.value(field, ordered_json{})) {
                std::ostringstream where;
                where << "[entries[" << i << "]." << field << "]";
                std::ostringstream msg;
                msg << "actual=" << a.value(field, ordered_json{}).dump()
                    << " expected=" << e.value(field, ordered_json{}).dump()
                    << " (name=" << a.value("name", std::string{}) << ")";
                note(where.str(), msg.str());
            }
        }
    }

    for (size_t i = pairCount; i < aEntries.size(); ++i) {
        std::ostringstream where; where << "[entries[" << i << "]]";
        note(where.str(), "extra in actual: " + aEntries[i].dump());
    }
    for (size_t i = pairCount; i < eEntries.size(); ++i) {
        std::ostringstream where; where << "[entries[" << i << "]]";
        note(where.str(), "missing from actual: " + eEntries[i].dump());
    }

    return diff.str();
}

fs::path FixturePath(std::string_view relative) {
    return fs::path(GOWTOOLKIT_TEST_FIXTURES_DIR) / std::string(relative);
}

bool ShouldUpdateGoldens() {
    const char* env = std::getenv("GOWTOOLKIT_GOLDEN_UPDATE");
    return env && std::string(env) == "1";
}

void RunGoldenTest(std::string_view versionTag,
                   const fs::path& wadPath,
                   const fs::path& expectedJsonPath) {
    REQUIRE_MESSAGE(fs::exists(wadPath),
                    "fixture WAD not found: " << wadPath.string());

    // TypeCatalog::Get() is the same process-wide singleton the app seeds
    // via Onyx::GameTypes::RegisterGameTypes() -- but the tree this test
    // parses no longer carries those legacy ids (Phase 2 Task 4 converged
    // WadNodeBuilder/Gow2Module onto each module's own TypeRegistrar-minted
    // handles), so registering the module on a fresh Workspace here is what
    // actually populates the labels Label(typeId) below reads. A new
    // Workspace per call is safe to repeat across TEST_CASEs sharing the
    // same process: TypeRegistrar::Add and TypeCatalog::Register are both
    // idempotent by key ("re-adding the same bare key returns the existing
    // handle"), and Workspace::AddModule's duplicate-id refusal is scoped
    // to one Workspace instance, not the catalog.
    Onyx::Types::TypeCatalog& catalog = Onyx::Types::TypeCatalog::Get();
    Onyx::Modules::Workspace workspace(catalog);

    if (versionTag == "gow2") {
        workspace.AddModule(std::make_unique<Onyx::Gow2::Gow2Module>());
    } else if (versionTag == "gowr") {
        workspace.AddModule(std::make_unique<Onyx::Gowr::GowrModule>());
    } else {
        FAIL("unknown versionTag: " << versionTag);
    }

    // Workspace::Open's synchronous path runs ParseContainer inline and
    // returns before this call does, so subscribing before Open() and
    // pumping right after is sufficient to observe this document's own
    // TreeReady -- no other document/job is in flight to interleave with.
    bool parsedOk = false;
    auto sub = workspace.Events().On<Onyx::Modules::TreeReady>(
        [&](const Onyx::Modules::TreeReady& e) { parsedOk = e.ok; });

    Onyx::Modules::DocumentId id = workspace.Open(wadPath, versionTag);
    workspace.Events().Pump();

    REQUIRE_MESSAGE(id != 0, "no module accepted " << wadPath.string()
                    << " (moduleHint=" << versionTag << ")");
    Onyx::Modules::Document* doc = workspace.Get(id);
    REQUIRE_MESSAGE(doc != nullptr,
                    "Workspace::Get returned no document for " << wadPath.string());
    REQUIRE_MESSAGE(parsedOk, "ParseContainer failed for " << wadPath.string());

    ordered_json actual = SnapshotEntries(*doc);

    if (ShouldUpdateGoldens()) {
        fs::create_directories(expectedJsonPath.parent_path());
        std::ofstream out(expectedJsonPath);
        REQUIRE_MESSAGE(out, "cannot write golden: " << expectedJsonPath.string());
        out << actual.dump(2) << '\n';
        MESSAGE("[golden-update] wrote " << expectedJsonPath.string());
        return;
    }

    ordered_json expected = LoadGolden(expectedJsonPath);
    REQUIRE_MESSAGE(!expected.is_null(),
                    "golden JSON missing — rerun with GOWTOOLKIT_GOLDEN_UPDATE=1 to create it: "
                    << expectedJsonPath.string());

    std::string diff = DiffSnapshots(actual, expected);
    if (!diff.empty()) {
        MESSAGE("Snapshot mismatch for " << wadPath.filename().string() << ":\n" << diff);
    }
    CHECK(diff.empty());
}

} // namespace gowtoolkit::testing
