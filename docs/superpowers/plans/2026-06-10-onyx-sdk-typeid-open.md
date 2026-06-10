# Onyx SDK — Open Type System (Plano 2: M1c) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the closed `enum class TypeId` with an opaque handle (`struct TypeId { uint32_t value; }`) plus a runtime `TypeCatalog` that the app populates with per-type metadata (key, label, `MediaKind`, icon, color), so the engine no longer hardcodes game-specific asset types — while keeping the single-target build and golden tests green.

**Architecture:** A new `TypeCatalog` registry holds `TypeInfo` per type. The GoW types become **named handle constants** in `Onyx::GameTypes`, registered at startup from a single data table that replaces the hardcoded `KindOf` / `TypeIdName` / `TypeVisuals` switches. The opaque handle keeps `operator==`/`std::hash` so the ~70 equality comparisons survive a mechanical `TypeId::X → GameTypes::X` rename. Persistence (GTKC V9 visibility overrides) stays byte-compatible **because the catalog registers types in the legacy enum order**, so each handle's `.value` equals its old enum value.

**Tech Stack:** C++20, CMake + Ninja, MSVC. doctest unit tests; golden snapshot tests are the primary regression net.

**Scope:** ONLY M1c (open the type system). No files move to `Engine/`/`Apps/` (that is M2, Plano 3). The `enum`-vs-handle flip is inherently atomic — Task 2 is one larger change verified by build + golden + the runtime equivalence test at its end; Tasks 1 and 3 bracket it with green, additive safety.

**Environment (this machine):** `cl` is not on PATH — enter the VS 2022 BuildTools dev shell IN THE SAME shell call as any build/test (env doesn't persist across tool calls):
```powershell
Import-Module "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools" -DevCmdArguments "-arch=x64 -host_arch=x64" -SkipAutomaticLocation
```
`build/` is already configured. Canonical cycle: `cmake --build build` then `ctest --test-dir build --output-on-failure` (7 tests; golden are self-contained). When committing, `git add` only `src/`, `tests/`, `CMakeLists.txt` by explicit path — never `git add -A` (uncommitted third_party/ deltas must stay out).

**IMPORTANT — commit messages:** Do NOT add any `Co-Authored-By` trailer or AI/generated-by credit to commits. (Standing user instruction.)

**Reference — the current type system map** (verified against the tree):
- `src/core/types/TypeId.h` — `enum class TypeId : uint32_t` (~48 values, `Unknown=0`, ends with `COUNT`). `TypeIdName()` declared here, defined in `TypeRegistry.cpp:112-166` (big switch).
- `src/core/domain/MediaKind.h:30-123` — `constexpr MediaKind KindOf(TypeId)` (big switch). Also has MediaKind→Name/Icon switches (keyed on `MediaKind`, a closed enum — leave those).
- `src/core/types/TypeRegistry.{h,cpp}` — dispatch by magic/tag; `m_idMap` keyed by `static_cast<uint32_t>(TypeId)`; `Resolve(TypeId)`.
- `src/core/types/ITypeHandler.h` — `virtual TypeId GetId() const = 0`; handlers return `TypeId::Mesh` etc.; `REGISTER_TYPE/TAG/FILE_TYPE` macros.
- `src/ui/TypeVisuals.h` — `TypeName`/`ColorForType`/`IconForType`, each a handler lookup + fallback `switch(typeId)`.
- Producers: `src/core/profiles/gow2/ProfileGOW2.cpp` (assigns `entry.typeId = TypeId::X`, calls `KindOf`), `src/core/profiles/gowr/WadNodeBuilder.cpp` (`RoleToTypeId(role)` 23-case switch returning `TypeId::X`).
- Consumers: `src/core/AssetVisibility.{h,cpp}` (keys by `static_cast<uint32_t>(id)`; `SerializedOverride.key` packs typeId in a `uint8_t`; sentinel `id==TypeId::Unknown||id==TypeId::COUNT`), `src/ui/WadBrowser.cpp`, `src/ui/PakBrowser.cpp`, `src/ui/viewers/MapViewer.cpp`, `src/cli/CliApp.cpp` (uses `TypeIdName`).
- Tests: `tests/mediakind_test.cpp` (`static_assert(KindOf(TypeId::Mesh)==MediaKind::Mesh)`…), `tests/viewer_registry_test.cpp` (`entry.typeId = TypeId::Unknown`).

---

## Pré-requisito: confirmar branch e baseline

- [ ] **Step 1: Branch + baseline green**

Run (PowerShell, dev shell + build in one call):
```
git checkout feat/onyx-genericization
```
Then build+test once to confirm the starting point is green:
```
cmake --build build ; ctest --test-dir build --output-on-failure
```
Expected: `100% tests passed` (7 tests). If not green, STOP — fix environment before proceeding.

---

## Task 1: Registration table + equivalence test (additive, stays green)

De-risks the flip: build the single data table that will seed the catalog, and a test proving it reproduces the existing `KindOf`/`TypeIdName` outputs AND that each type's intended numeric id equals its legacy enum value (the persistence-stability invariant). Everything still compiles because `TypeId` is still the enum here.

**Files:**
- Create: `src/core/types/GameTypeTable.h`
- Create: `tests/typetable_equivalence_test.cpp`
- Modify: `CMakeLists.txt` (add the test source to the test target; add `GameTypeTable` has no .cpp yet — header-only table)
- Modify: `tests/CMakeLists.txt` (register the new test file — inspect how existing test files are listed and follow the pattern)

- [ ] **Step 1: Write the table header**

Create `src/core/types/GameTypeTable.h`. Transcribe EVERY enum value from `TypeId.h` in declaration order into one row each, filling `media` from `MediaKind.h`'s `KindOf` switch and `label` from `TypeRegistry.cpp`'s `TypeIdName` switch. Icon/color: use `ICON_SF_DOCUMENT` and `{0.6f,0.6f,0.6f,1.0f}` as defaults EXCEPT where `TypeVisuals.h`'s fallback switches specify a distinct icon/color (transcribe those). Structure:

```cpp
#pragma once
#include "core/types/TypeId.h"        // still the enum at this point
#include "core/domain/MediaKind.h"
#include "fonts/SFSymbols.h"
#include <cstdint>

namespace Onyx {

// One row per asset type. `legacyValue` MUST equal the type's position in the
// old `enum class TypeId` (Unknown=0, EntityCount=1, ...). This locks GTKC V9
// persistence stability: handles registered in this order get .value ==
// legacyValue, so saved visibility overrides keep round-tripping.
struct GameTypeRow {
    uint32_t     legacyValue;   // == old enum numeric value
    const char*  key;           // stable string id, e.g. "GOW2_MESH"
    const char*  label;         // == old TypeIdName()
    MediaKind    media;         // == old KindOf()
    const char*  icon;          // == old IconForType() (or ICON_SF_DOCUMENT)
    float        color[4];      // == old ColorForType() (or {.6,.6,.6,1})
};

// Order MUST match enum declaration order in TypeId.h. Do NOT include COUNT.
inline constexpr GameTypeRow kGameTypeTable[] = {
    { 0,  "UNKNOWN",       "Unknown",      MediaKind::Unknown, ICON_SF_DOCUMENT, {0.6f,0.6f,0.6f,1.0f} },
    { 1,  "ENTITY_COUNT",  "EntityCount",  MediaKind::Unknown, ICON_SF_DOCUMENT, {0.6f,0.6f,0.6f,1.0f} },
    // ... transcribe ALL remaining enum values, in order, through Sentinel ...
    // e.g.:
    // { 9, "GOW2_MESH", "Mesh", MediaKind::Mesh, <icon from TypeVisuals or default>, <color> },
};

} // namespace Onyx
```

Fill in EVERY row (do not leave the `...`). Cross-check counts: number of rows must equal `(int)TypeId::COUNT`.

- [ ] **Step 2: Write the equivalence test**

Create `tests/typetable_equivalence_test.cpp`:

```cpp
#include "doctest.h"
#include "core/types/GameTypeTable.h"
#include "core/types/TypeId.h"
#include "core/domain/MediaKind.h"
#include <cstring>

using namespace Onyx;

TEST_CASE("GameTypeTable reproduces legacy KindOf and TypeIdName") {
    // Row count matches the enum (excluding COUNT).
    CHECK((int)(sizeof(kGameTypeTable)/sizeof(kGameTypeTable[0])) == (int)TypeId::COUNT);

    for (const auto& row : kGameTypeTable) {
        TypeId legacy = static_cast<TypeId>(row.legacyValue);
        // legacyValue is exactly the enum numeric value
        CHECK((uint32_t)legacy == row.legacyValue);
        // media metadata matches the old constexpr switch
        CHECK(KindOf(legacy) == row.media);
        // label metadata matches the old TypeIdName switch
        CHECK(std::strcmp(TypeIdName(legacy), row.label) == 0);
    }
}
```

- [ ] **Step 3: Register the test in the build**

Read `tests/CMakeLists.txt`, find where test `.cpp` files are added to the `gowtoolkit_tests` target, and add `typetable_equivalence_test.cpp` the same way. (Do NOT invent a new mechanism — match the existing list.)

- [ ] **Step 4: Build + run the new test**

Run: `cmake --build build ; ctest --test-dir build -R unit --output-on-failure`
Expected: passes, including the new `GameTypeTable` case. If `TypeIdName`/`KindOf` mismatches surface, the table has a wrong row — fix the table (it is the source of truth going forward). Iterate until green. This step is the whole point: it proves the table is a faithful, complete replacement for the switches.

- [ ] **Step 5: Full suite + commit**

Run: `ctest --test-dir build --output-on-failure` → `100% tests passed`.
```
git add src/core/types/GameTypeTable.h tests/typetable_equivalence_test.cpp tests/CMakeLists.txt
git commit -m "test(types): Add type-metadata table with legacy-equivalence proof"
```

---

## Task 2: The flip — opaque handle + TypeCatalog (atomic; verified at end)

Replace the enum with the handle and route all metadata through the catalog. Large but coherent; the build is expected red mid-task and green only at the end (Steps build/test are the gate).

**Files:**
- Modify: `src/core/types/TypeId.h` (enum → struct handle; drop `TypeIdName` decl)
- Create: `src/core/types/TypeCatalog.h`, `src/core/types/TypeCatalog.cpp`
- Create: `src/core/types/GameTypes.h`, `src/core/types/GameTypes.cpp`
- Modify: `src/core/domain/MediaKind.h` (remove `KindOf(TypeId)` + its TypeId include; keep `enum MediaKind` and the MediaKind→Name/Icon switches)
- Modify: `src/core/types/TypeRegistry.cpp` (delete `TypeIdName` switch; `static_cast<uint32_t>(id)` → `id.value`)
- Modify: `src/ui/TypeVisuals.h` (3 switches → catalog lookups, keep handler override)
- Modify: `src/core/AssetVisibility.{h,cpp}` (`static_cast<uint32_t>(id)` → `id.value`; `static_cast<TypeId>(u)` → `TypeId{u}`; sentinel; `MakeKey`)
- Modify (scripted): all `TypeId::X` usages → `GameTypes::X` across `src/` and `tests/`
- Modify: `CMakeLists.txt` (add `TypeCatalog.cpp`, `GameTypes.cpp` to `PARSER_MIN_SOURCES`)
- Modify: `tests/mediakind_test.cpp`, `tests/viewer_registry_test.cpp`

- [ ] **Step 1: Replace the enum with the opaque handle**

Overwrite `src/core/types/TypeId.h`:

```cpp
#pragma once
#include <cstdint>
#include <functional>

namespace Onyx {

// Opaque, runtime-assigned asset-type identity. value 0 == Unknown/invalid.
// Concrete types live in the app and are registered in the TypeCatalog
// (see GameTypes.h); the engine never enumerates them.
struct TypeId {
    uint32_t value = 0;
    constexpr bool operator==(const TypeId& o) const { return value == o.value; }
    constexpr bool operator!=(const TypeId& o) const { return value != o.value; }
    constexpr bool valid() const { return value != 0; }
};

} // namespace Onyx

template<> struct std::hash<Onyx::TypeId> {
    size_t operator()(const Onyx::TypeId& t) const noexcept {
        return std::hash<uint32_t>()(t.value);
    }
};
```

(Note: `TypeIdName` is gone — its replacement is `TypeCatalog::Label`. `COUNT` is gone — handled in Step 8.)

- [ ] **Step 2: Add TypeCatalog**

Create `src/core/types/TypeCatalog.h`:

```cpp
#pragma once
#include "TypeId.h"
#include "core/domain/MediaKind.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace Onyx {

struct TypeInfo {
    std::string key;                      // stable id, e.g. "GOW2_MESH"
    std::string label;                    // human-readable
    MediaKind   media = MediaKind::Unknown;
    const char* icon  = nullptr;          // UTF-8 codicon; nullptr => default
    float       color[4] = {0.6f,0.6f,0.6f,1.0f};
};

// Open registry: app registers its types; engine routes by metadata only.
class TypeCatalog {
public:
    static TypeCatalog& Get();

    // Registers a type and returns its handle. If forcedValue != 0, the handle
    // takes that exact value (used to preserve legacy enum values for GTKC
    // persistence stability). Re-registering the same key returns the existing
    // handle.
    TypeId Register(const TypeInfo& info, uint32_t forcedValue = 0);

    TypeId            Find(std::string_view key) const;   // {} if absent
    const TypeInfo&   Info(TypeId id) const;              // Unknown info if absent
    MediaKind         Media(TypeId id) const { return Info(id).media; }
    const char*       Label(TypeId id) const { return Info(id).label.c_str(); }
    const char*       Icon(TypeId id) const;              // default if none
    void              Color(TypeId id, float out[4]) const;

private:
    TypeCatalog();
    std::vector<TypeInfo>                  m_infos;   // index = TypeId.value
    std::unordered_map<std::string, TypeId> m_byKey;
    TypeInfo                                m_unknown; // index 0
};

// Convenience free function preserving the old call sites' spelling.
inline MediaKind KindOf(TypeId id) { return TypeCatalog::Get().Media(id); }

} // namespace Onyx
```

Create `src/core/types/TypeCatalog.cpp`:

```cpp
#include "core/types/TypeCatalog.h"
#include "fonts/SFSymbols.h"

namespace Onyx {

TypeCatalog& TypeCatalog::Get() { static TypeCatalog inst; return inst; }

TypeCatalog::TypeCatalog() {
    m_unknown.key = "UNKNOWN";
    m_unknown.label = "Unknown";
    m_infos.push_back(m_unknown);            // index 0 == Unknown
    m_byKey[m_unknown.key] = TypeId{0};
}

TypeId TypeCatalog::Register(const TypeInfo& info, uint32_t forcedValue) {
    if (auto it = m_byKey.find(info.key); it != m_byKey.end()) return it->second;
    uint32_t v = forcedValue ? forcedValue : (uint32_t)m_infos.size();
    if (v >= m_infos.size()) m_infos.resize(v + 1);
    m_infos[v] = info;
    TypeId id{v};
    m_byKey[info.key] = id;
    return id;
}

TypeId TypeCatalog::Find(std::string_view key) const {
    auto it = m_byKey.find(std::string(key));
    return it == m_byKey.end() ? TypeId{} : it->second;
}

const TypeInfo& TypeCatalog::Info(TypeId id) const {
    return (id.value < m_infos.size()) ? m_infos[id.value] : m_unknown;
}

const char* TypeCatalog::Icon(TypeId id) const {
    const char* ic = Info(id).icon;
    return ic ? ic : ICON_SF_DOCUMENT;
}

void TypeCatalog::Color(TypeId id, float out[4]) const {
    const auto& c = Info(id).color;
    out[0]=c[0]; out[1]=c[1]; out[2]=c[2]; out[3]=c[3];
}

} // namespace Onyx
```

- [ ] **Step 3: Add GameTypes (named handles + registration from the table)**

Create `src/core/types/GameTypes.h`:

```cpp
#pragma once
#include "core/types/TypeId.h"

namespace Onyx::GameTypes {

// Named handles for the GoW asset types. Valid after RegisterGameTypes() runs
// (called once at startup, before any parse). Declared extern; defined in
// GameTypes.cpp. Names match the legacy TypeId enum values one-for-one.
extern TypeId Unknown, EntityCount, GroupStart, GroupEnd, HeaderStart, HeaderPop,
    Instance, Object, Model, Mesh, Material, Texture, GfxData, PalData,
    Animation, Script, Light, Sound, Collision, Flipbook, Chunk,
    WadFile, VagAudio, VpkVideo, PssVideo, PswVideo, TextPlain,
    ShaderContainer, ShaderVertex, ShaderPixel, ShaderHull, ShaderDomain,
    ShaderCompute, ShaderLibrary, MeshGpu, MeshDefn, GameObjectProto,
    GameObjectInst, GameObjectOverride, TexturePair, MaterialRef, LodBinding,
    AnimClip, SoundEmitter, ParticleEmitter, ParticleSystem, ClientGuid,
    WadIdentity, SharedWadRef, Sentinel;

// Registers every GoW type into the TypeCatalog, in legacy enum order so each
// handle's .value equals its old enum value (GTKC V9 persistence stability).
// Idempotent. Call once at program startup (see Main wiring).
void RegisterGameTypes();

} // namespace Onyx::GameTypes
```

Create `src/core/types/GameTypes.cpp`:

```cpp
#include "core/types/GameTypes.h"
#include "core/types/TypeCatalog.h"
#include "core/types/GameTypeTable.h"

namespace Onyx::GameTypes {

TypeId Unknown, EntityCount, GroupStart, GroupEnd, HeaderStart, HeaderPop,
    Instance, Object, Model, Mesh, Material, Texture, GfxData, PalData,
    Animation, Script, Light, Sound, Collision, Flipbook, Chunk,
    WadFile, VagAudio, VpkVideo, PssVideo, PswVideo, TextPlain,
    ShaderContainer, ShaderVertex, ShaderPixel, ShaderHull, ShaderDomain,
    ShaderCompute, ShaderLibrary, MeshGpu, MeshDefn, GameObjectProto,
    GameObjectInst, GameObjectOverride, TexturePair, MaterialRef, LodBinding,
    AnimClip, SoundEmitter, ParticleEmitter, ParticleSystem, ClientGuid,
    WadIdentity, SharedWadRef, Sentinel;

void RegisterGameTypes() {
    // Map legacy value -> the extern handle, so registration fills them in order.
    TypeId* slots[] = {
        &Unknown, &EntityCount, &GroupStart, &GroupEnd, &HeaderStart, &HeaderPop,
        &Instance, &Object, &Model, &Mesh, &Material, &Texture, &GfxData, &PalData,
        &Animation, &Script, &Light, &Sound, &Collision, &Flipbook, &Chunk,
        &WadFile, &VagAudio, &VpkVideo, &PssVideo, &PswVideo, &TextPlain,
        &ShaderContainer, &ShaderVertex, &ShaderPixel, &ShaderHull, &ShaderDomain,
        &ShaderCompute, &ShaderLibrary, &MeshGpu, &MeshDefn, &GameObjectProto,
        &GameObjectInst, &GameObjectOverride, &TexturePair, &MaterialRef, &LodBinding,
        &AnimClip, &SoundEmitter, &ParticleEmitter, &ParticleSystem, &ClientGuid,
        &WadIdentity, &SharedWadRef, &Sentinel,
    };
    auto& cat = TypeCatalog::Get();
    for (const auto& row : kGameTypeTable) {
        TypeInfo info;
        info.key = row.key; info.label = row.label; info.media = row.media;
        info.icon = row.icon;
        info.color[0]=row.color[0]; info.color[1]=row.color[1];
        info.color[2]=row.color[2]; info.color[3]=row.color[3];
        TypeId id = cat.Register(info, row.legacyValue);  // force value == legacy
        *slots[row.legacyValue] = id;
    }
}

} // namespace Onyx::GameTypes
```

> Order check: the `slots[]` array MUST list the handles in the exact legacy enum order (index == legacyValue). `GameTypeTable.h` was authored in that order in Task 1, so `row.legacyValue` indexes `slots[]` correctly.

- [ ] **Step 4: Make MediaKind.h stop owning `KindOf(TypeId)`**

In `src/core/domain/MediaKind.h`: delete the `#include` of `TypeId.h` and the entire `constexpr MediaKind KindOf(TypeId)` function (lines ~30-123). KEEP `enum class MediaKind` and any `Name(MediaKind)`/`Icon(MediaKind)` switches (they key on the closed enum). Callers of `KindOf` now get it from `TypeCatalog.h` (the inline free function added in Step 2) — they must include `core/types/TypeCatalog.h`. Add that include to the producer files that call `KindOf`: `src/core/profiles/gow2/ProfileGOW2.cpp` and `src/core/profiles/gowr/WadNodeBuilder.cpp` (and any other file the build flags as missing `KindOf`).

- [ ] **Step 5: Delete the `TypeIdName` switch in TypeRegistry.cpp**

Remove the `TypeIdName` function definition (`TypeRegistry.cpp:112-166`). Replace its call sites: in `src/cli/CliApp.cpp` and `src/ui/TypeVisuals.h`, `Onyx::TypeIdName(x)` → `Onyx::TypeCatalog::Get().Label(x)` (add `#include "core/types/TypeCatalog.h"` where needed). In `TypeRegistry.cpp`/`.h`, replace `static_cast<uint32_t>(id)` (and `static_cast<uint32_t>(raw->GetId())`) with `id.value` / `raw->GetId().value` (4 sites per the map: lines ~29, 54, 70, 106).

- [ ] **Step 6: Convert TypeVisuals.h switches to catalog lookups**

Rewrite the three functions in `src/ui/TypeVisuals.h` to: first try the handler (`TypeRegistry::Get().Resolve(typeId)` → `GetName/GetIcon/GetColor`) as today, then fall back to the **catalog** instead of a `switch`:

```cpp
inline const char* TypeName(TypeId typeId) {
    if (auto* h = TypeRegistry::Get().Resolve(typeId)) return h->GetName();
    return TypeCatalog::Get().Label(typeId);
}
inline const char* IconForType(TypeId typeId) {
    if (auto* h = TypeRegistry::Get().Resolve(typeId)) return h->GetIcon();
    return TypeCatalog::Get().Icon(typeId);
}
inline ImVec4 ColorForType(TypeId typeId) {           // match the file's actual return type
    if (auto* h = TypeRegistry::Get().Resolve(typeId)) { auto c = h->GetColor(); return {c.r,c.g,c.b,c.a}; }
    float c[4]; TypeCatalog::Get().Color(typeId, c);   return {c[0],c[1],c[2],c[3]};
}
```
(Match the existing return types/signatures in the file — check whether it returns `ImVec4` or the handler's `Color4f`. Add `#include "core/types/TypeCatalog.h"`.)

- [ ] **Step 7: Scripted rename `TypeId::X` → `GameTypes::X`**

This converts every enum-member access (handlers' `GetId()` returns, the ~70 comparisons, producers' assignments, `RoleToTypeId` returns) to the named handles. The type name `TypeId` (used as `TypeId typeId`, `TypeId GetId()`) is NOT followed by `::`, so it is untouched.

```powershell
$enc = New-Object System.Text.UTF8Encoding($false)
Get-ChildItem -Path src,tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m | ForEach-Object {
  $c = [IO.File]::ReadAllText($_.FullName)
  $n = [regex]::Replace($c, '\bTypeId::', 'GameTypes::')
  if ($n -ne $c) { [IO.File]::WriteAllText($_.FullName, $n, $enc) }
}
```
After this, add `#include "core/types/GameTypes.h"` to files that now reference `GameTypes::` but didn't include it (the build will flag them — typically the handlers, `ProfileGOW2.cpp`, `WadNodeBuilder.cpp`, `AssetVisibility.cpp`, `WadBrowser.cpp`, `PakBrowser.cpp`, `MapViewer.cpp`, `TypeVisuals.h`).

- [ ] **Step 8: Fix the two `COUNT` sentinels and the inverse cast**

`GameTypes::COUNT` does not exist (the script produced it from `TypeId::COUNT`). Fix:
- `src/core/domain/MediaKind.h` — the `case TypeId::COUNT` line was inside the `KindOf` switch already deleted in Step 4, so nothing to do there; verify it's gone.
- `src/core/AssetVisibility.cpp:125` — `if (id == GameTypes::COUNT || id == GameTypes::Unknown)` → `if (!id.valid())` (i.e., `id == GameTypes::Unknown` since Unknown is value 0; drop the COUNT term).
- `src/core/AssetVisibility.cpp:166` — `static_cast<TypeId>(typeId)` → `TypeId{typeId}`.
- `src/core/AssetVisibility.h:73-74` — `MakeKey`: `static_cast<uint32_t>(id)` → `id.value`.
- Search for any other `static_cast<uint32_t>(` applied to a TypeId and any remaining `GameTypes::COUNT`:
  ```powershell
  Select-String -Path (Get-ChildItem -Path src,tests -Recurse -Include *.h,*.cpp,*.hpp).FullName -Pattern 'GameTypes::COUNT','static_cast<uint32_t>\(.*[tT]ypeId','static_cast<TypeId>'
  ```
  Resolve each: `GameTypes::COUNT` → remove/replace with `!id.valid()` logic; `static_cast<uint32_t>(x)` on a TypeId → `x.value`; `static_cast<TypeId>(u)` → `TypeId{u}`.

- [ ] **Step 9: Wire `RegisterGameTypes()` at startup**

The catalog must be populated before any parse or UI draw. Add a call to `Onyx::GameTypes::RegisterGameTypes();` early in both entry points: in `src/main.cpp` (right after profiles are registered, before the window loop) and in `src/cli/CliApp.cpp` (at the start of its run). Add `#include "core/types/GameTypes.h"` to both.

- [ ] **Step 10: Update the two affected tests**

`tests/mediakind_test.cpp`: `static_assert(KindOf(...))` no longer compiles (KindOf is runtime now). Convert to a runtime test that first calls `RegisterGameTypes()` then checks the catalog:
```cpp
#include "doctest.h"
#include "core/types/GameTypes.h"
#include "core/types/TypeCatalog.h"
using namespace Onyx;
TEST_CASE("Catalog media routing") {
    GameTypes::RegisterGameTypes();
    CHECK(KindOf(GameTypes::Mesh)     == MediaKind::Mesh);
    CHECK(KindOf(GameTypes::Texture)  == MediaKind::Image);
    CHECK(KindOf(GameTypes::Sound)    == MediaKind::Audio);
    CHECK(KindOf(GameTypes::Instance) == MediaKind::Map);
}
```
`tests/viewer_registry_test.cpp`: `entry.typeId = TypeId::Unknown` was rewritten to `GameTypes::Unknown` by Step 7; add `GameTypes::RegisterGameTypes();` at the top of that test case and `#include "core/types/GameTypes.h"` so the handle is valid. Also update `tests/typetable_equivalence_test.cpp` from Task 1: `static_cast<TypeId>(row.legacyValue)` → `TypeId{row.legacyValue}`, `(uint32_t)legacy` → `legacy.value`, `(int)TypeId::COUNT` → `(int)(sizeof(kGameTypeTable)/sizeof(kGameTypeTable[0]))` (COUNT is gone), and replace the `KindOf(legacy)`/`TypeIdName(legacy)` legacy comparisons with catalog checks after `RegisterGameTypes()`:
```cpp
GameTypes::RegisterGameTypes();
for (const auto& row : kGameTypeTable) {
    TypeId id{row.legacyValue};
    CHECK(TypeCatalog::Get().Media(id) == row.media);
    CHECK(std::strcmp(TypeCatalog::Get().Label(id), row.label) == 0);
}
```

- [ ] **Step 11: Add the new sources to CMake**

In `CMakeLists.txt`, add to the `PARSER_MIN_SOURCES` list (so both the test target and the main exe link them):
```
src/core/types/TypeCatalog.cpp
src/core/types/GameTypes.cpp
```

- [ ] **Step 12: Build**

Run: `cmake --build build`
Expected: compiles. Work through errors iteratively — typical ones: missing `#include "core/types/GameTypes.h"` or `TypeCatalog.h`; a stray `GameTypes::COUNT`; a `static_cast` not yet converted; a handler `.cpp` whose `GetId()` returns `GameTypes::X` but lacks the include. Fix each at its site.

- [ ] **Step 13: Full test suite**

Run: `ctest --test-dir build --output-on-failure`
Expected: `100% tests passed`. The golden tests (GOW2/GOWR) are the key regression guard — they re-parse fixtures and compare the full tree including each entry's type/kind. If a golden diff appears, a type's metadata or assignment changed — diff against the table and fix. The runtime equivalence + media-routing tests must also pass.

- [ ] **Step 14: Commit**

```
git add src tests CMakeLists.txt
git commit -m "refactor(types): Replace TypeId enum with opaque handle and TypeCatalog"
```

---

## Task 3: Persistence stability regression check

Prove the GTKC V9 visibility-override round-trip still works after the handle flip — the whole point of forcing legacy values.

**Files:**
- Create: `tests/visibility_persistence_test.cpp`
- Modify: `tests/CMakeLists.txt` (register it)

- [ ] **Step 1: Write the round-trip test**

Create `tests/visibility_persistence_test.cpp`:

```cpp
#include "doctest.h"
#include "core/AssetVisibility.h"
#include "core/types/GameTypes.h"
#include "core/types/TypeCatalog.h"
using namespace Onyx;

TEST_CASE("Visibility override survives Export/Import by stable value") {
    GameTypes::RegisterGameTypes();
    // A registered handle's value must equal its legacy enum position so the
    // uint8 SerializedOverride.key stays byte-compatible with old gowtool.cfg.
    CHECK(GameTypes::Mesh.value     == 9);   // adjust to the actual legacy value
    CHECK(GameTypes::Texture.value  == 11);  // adjust to the actual legacy value

    auto& vis = AssetVisibility::Get();
    vis.ResetAllOverrides();
    vis.SetUserOverride(GameVersion::GOW2, GameTypes::Mesh, /*visible=*/false);

    auto blob = vis.ExportOverrides();
    REQUIRE(blob.size() == 1);
    vis.ResetAllOverrides();
    vis.ImportOverrides(blob);

    CHECK(vis.GetCurrent(GameVersion::GOW2, GameTypes::Mesh) == Visibility::Hidden
          || vis.IsVisible(GameVersion::GOW2, GameTypes::Mesh) == false);
}
```
(Set the two `.value` literals to the actual legacy positions of Mesh/Texture from `TypeId.h`'s original order — count them. These assertions lock the stability invariant.)

- [ ] **Step 2: Register + build + run**

Add the file to `tests/CMakeLists.txt` (match existing pattern). Run:
`cmake --build build ; ctest --test-dir build --output-on-failure`
Expected: `100% tests passed`, including the persistence case.

- [ ] **Step 3: Commit**

```
git add tests/visibility_persistence_test.cpp tests/CMakeLists.txt
git commit -m "test(types): Lock GTKC visibility-override value stability"
```

---

## Verificação final do Plano 2

- [ ] **Step 1: Clean rebuild + full suite**

Run: `cmake --build build --target clean ; cmake --build build ; ctest --test-dir build --output-on-failure`
Expected: rebuilds clean; `100% tests passed`.

- [ ] **Step 2: Confirm the enum is gone and the engine no longer switches on game types**

Run (PowerShell):
```powershell
Select-String -Path (Get-ChildItem -Path src,tests -Recurse -Include *.h,*.hpp,*.cpp).FullName -Pattern 'enum class TypeId','case TypeId::','TypeId::COUNT','GameTypes::COUNT' | Select-Object -First 20
```
Expected: no lines (the enum, its case labels, and COUNT are all gone).

- [ ] **Step 3: Confirm new structure present**

Run:
```powershell
Select-String -Path (Get-ChildItem -Path src -Recurse -Include *.h,*.cpp).FullName -Pattern 'struct TypeId','class TypeCatalog','RegisterGameTypes' | Select-Object -First 10
```
Expected: shows the handle, catalog, and registration entry point. Plano 2 concluído.

---

## Roadmap dos planos seguintes (não implementar aqui)

- **Plano 3 — Split engine/app + PascalCase + MinimalViewer (M2 + M3 + M4).** Move files into `Engine/` (incl. `TypeCatalog`, generic viewers, VFS, schema) vs `Apps/GoWToolkit/` (profiles, parsers, handlers, `GameTypes`/`GameTypeTable`, game viewers). Now that type identity is opaque and game types are registered from an app-owned table, `GameTypes.*`/`GameTypeTable.h` move cleanly to the app; the engine keeps only `TypeId`+`TypeCatalog`+`MediaKind`. Then PascalCase dirs + `Include/Onyx/` public surface + `MinimalViewer` (2nd consumer that registers its own toy types — proves the catalog is truly open).
- **Plano 4 — Physical repo split (M5).** `git filter-repo` extract `Engine/` → `OnyxSDK`; FetchContent; tag.

### Carry-forward notes
- Settle the final SDK name before Plano 4 (`Onyx` is placeholder).
- The `IGameProfile.h` filename still holds `IAssetProfile` (deferred file rename) — do it during the Plano 3 move.
- `tests/fixtures/README.md` prose was updated in Plano 1; re-check after the Plano 3 move.
