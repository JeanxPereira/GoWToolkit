#pragma once

// Golden-test plumbing: produce a stable JSON snapshot of an `OpenWad`,
// load a previously-recorded snapshot from disk, and diff the two with a
// human-readable error message. Tests in this directory use these helpers
// to verify that parser refactors don't silently change observable output.

#include <filesystem>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "core/WadTypes.h"

namespace gowtoolkit::testing {

// Returns a JSON document describing the relevant fields of every entry in
// `wad`. The shape is deliberately small and stable:
//
//   {
//     "wad": "<filename>",
//     "entryCount": N,
//     "entries": [
//       { "name": "...", "typeId": "...",
//         "size": 1234, "offset": 16, "childCount": 0,
//         "payloadHash": "0xXXXXXXXXXXXXXXXX" },
//       ...
//     ]
//   }
//
// Entries are flattened (no nesting) and ordered by `offset` ascending so
// the document is stable across runs even if the parser walks the tree in
// a different order. `payloadHash` is xxhash64 of the on-disk payload
// bytes (skipped when the entry has `size == 0`).
nlohmann::ordered_json SnapshotEntries(const OpenWad& wad);

// Reads a golden snapshot from disk. Returns `nullptr` JSON if the file
// does not exist (so callers can decide to create one on update mode).
nlohmann::ordered_json LoadGolden(const std::filesystem::path& path);

// Compares `actual` against `expected`. On mismatch, returns a multi-line
// human-readable description of every diff. Returns empty string when
// they match.
std::string DiffSnapshots(const nlohmann::ordered_json& actual,
                          const nlohmann::ordered_json& expected);

// Helper: returns the absolute path of a fixture file inside the
// `tests/fixtures` directory. The directory location is baked into the
// test binary via the `GOWTOOLKIT_TEST_FIXTURES_DIR` macro.
std::filesystem::path FixturePath(std::string_view relative);

// When `true`, golden tests overwrite their `*.expected.json` with the
// freshly computed snapshot instead of comparing. Toggled by setting the
// `GOWTOOLKIT_GOLDEN_UPDATE` environment variable to `1`.
bool ShouldUpdateGoldens();

// Convenience wrapper used by individual test cases: parse `wadPath` with
// the appropriate profile, snapshot the result, then either compare against
// `expectedJsonPath` or overwrite it. `versionTag` is "gow2" or "gowr".
void RunGoldenTest(std::string_view versionTag,
                   const std::filesystem::path& wadPath,
                   const std::filesystem::path& expectedJsonPath);

} // namespace gowtoolkit::testing
