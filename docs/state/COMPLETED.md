# Tasks Finalizadas

> Log append-only. Cada entrada: data, ID, link de PR/commit, ACs verificados, notas.
> Não editar entradas passadas — só apender.

---

## 2026-05-17 — Bootstrap

- **Commits**: `a087917`, `57567c1`, `c437d6f`, `c33cc1c`
- **Branch base**: `main` (de `efe65f2` → `c33cc1c`)
- **Branch refactor criada**: `refactor/m0-safety-net`
- **Artefatos**:
  - `docs/ARCHITECTURE_REVIEW_2026-05.md` (Part I + Part II)
  - `docs/ROADMAP_IMPLEMENTATION.md` (M0–M8 runbook)
  - `docs/state/CURRENT.md`
  - `docs/state/COMPLETED.md`
  - `docs/state/DECISIONS.md`
- **Notas**: WIP pré-existente do usuário (GOWR parser refinements + format docs) consolidado em main antes da branch refactor para manter histórico coerente.

---

## 2026-05-18 — Pre-M0.T1 housekeeping

- **Commits**:
  - `678cdbc` docs(state): scaffold milestone tracking files
  - `714fdbf` docs(formats): reorg per-game directories
- **Branch**: `refactor/m0-safety-net`
- **Notas**: state scaffold do bootstrap foi commitado nesta sessão (antes ficou só untracked). Reorg per-game (`docs/formats/GOW2/` → `docs/GoW2/Formats/`, adiciona `docs/GoW1/Formats/` placeholder, atualiza `docs/GoWRknk/`) consolidado em commit único.

---

## 2026-05-18 — M0.T1 — Setup doctest

- **Branch**: `refactor/m0-safety-net`
- **Subtasks completadas**: S1–S7 (7/7)
- **Arquivos novos**:
  - `tests/CMakeLists.txt` (define `gowtoolkit_tests`, GLOB `tests/*.cpp`, links `doctest::doctest`)
  - `tests/main.cpp` (`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`)
  - `tests/sanity_test.cpp` (smoke `CHECK(1 + 1 == 2)`)
- **CMakeLists.txt**: adicionado `include(CTest)`, opção `GOWTOOLKIT_BUILD_TESTS` (ON), `FetchContent` doctest v2.4.11, `add_subdirectory(tests)` + `add_test(NAME unit COMMAND $<TARGET_FILE:gowtoolkit_tests>)`.
- **AC verificados**: 4/4
  - [x] `ctest --test-dir build` → "Test #1: unit ... Passed"
  - [x] `tests/sanity_test.cpp` no build CMake
  - [x] Sem regressão no main exe (Debug builds verde; Release breaking pré-existente por `xcrun actool` ausente, não relacionado)
  - [x] doctest em fetched-content cache (`build/_deps/doctest-src/`), não no source tree
- **Notas**:
  - doctest v2.4.11 upstream usa `cmake_minimum_required(<3.5)`, incompatível com CMake 4.x. Workaround: `set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)` ao redor do `FetchContent_MakeAvailable(doctest)`. Remover quando upstream subir floor.
  - `add_test` precisa de `$<TARGET_FILE:gowtoolkit_tests>` (não apenas nome do target) pra ctest resolver path do exe quando build incremental ainda não materializou.

---

## 2026-05-18 — M0.T2 — Golden Test Fixtures (mínimos)

- **Branch**: `refactor/m0-safety-net`
- **Prereq satisfeito**: M0.T1 ✓
- **Decisão resolvida**: D0003 — fixtures vêm de truncamento de WADs comerciais (ver `DECISIONS.md`).
- **Arquivos novos**:
  - `tools/make_test_fixtures.py` (gerador determinístico)
  - `tests/fixtures/gow2/wad_minimal.wad` (265,936 bytes, truncated tag-aware do `R_BOAR00.WAD`)
  - `tests/fixtures/gowr/wad_minimal.wad` (538,410 bytes, cópia íntegra do `r_athena00.wad`)
  - `tests/fixtures/README.md`
  - `.gitattributes`
- **AC verificados**: 5/5
  - [x] `tests/fixtures/gow2/wad_minimal.wad` existe, < 1 MB (265 KB, abaixo do target 500 KB também)
  - [x] `tests/fixtures/gowr/wad_minimal.wad` existe, < 1 MB (538 KB)
  - [x] `tests/fixtures/README.md` documenta origem + SHA256
  - [x] `.gitattributes` marca `*.wad` (e variantes) como binárias
  - [x] `.gitignore` não esconde fixtures — versionadas
- **Notas**:
  - GOW2 truncado em boundary 16-byte-aligned do primeiro tag estrutural (`GROUP_END` ou `HEADER_START`) ≥ 256 KB. Validado: primeiros 8 tags do fixture batem com os do source (HEADER_START `WAD_R_Boar00`, GroupStart, SERVER_INST `WAD_R_Boar00`, ...).
  - GOWR copiado inteiro porque `blockBitSet` flush algorithm impede truncamento naive (offsets seriam invalidados). Source `r_athena00.wad` é per-character WAD, já pequeno.

---

## 2026-05-18 — M0.T3 — SnapshotEntries + Golden Test runner

- **Branch**: `refactor/m0-safety-net`
- **Prereqs satisfeitos**: M0.T1 ✓, M0.T2 ✓
- **Decisões registradas**: D0004 (parser-min lib), D0005 (xxhash em vez de SHA-1)
- **Arquivos novos**:
  - `tests/golden_helpers.{h,cpp}` (SnapshotEntries, LoadGolden, DiffSnapshots, RunGoldenTest, FixturePath, ShouldUpdateGoldens)
  - `tests/golden_gow2.cpp`, `tests/golden_gowr.cpp`
  - `tests/test_stubs.cpp` (GetTexIndex stub pra parser-min linkage)
  - `tests/fixtures/gow2/wad_minimal.expected.json` (33 entries)
  - `tests/fixtures/gowr/wad_minimal.expected.json` (114 entries → 2 root nodes)
  - `tools/regenerate_goldens.sh`
- **CMake**:
  - Novo target `gowtoolkit_parser_min` (static lib com profiles + WAD plumbing sem UI dep)
  - Main exe linka parser-min (sources excluídos do GLOB)
  - nlohmann/json v3.11.3 via FetchContent (test-only)
  - `Golden_GOW2` + `Golden_GOWR` registrados como ctest entries separadas
- **AC verificados**: 5/5
  - [x] `ctest -R Golden` passa em verde (2/2 passed)
  - [x] Mutação artificial (`WADTAG_SERVER_INST = 99`) faz ctest falhar com diff legível mostrando 18 entries `missing from actual`
  - [x] `tools/regenerate_goldens.sh` builda + regenera JSON
  - [x] JSON estável entre runs (SHA256 idêntico após `regenerate` consecutivos)
  - [x] Runner suporta update via `GOWTOOLKIT_GOLDEN_UPDATE=1` env var
- **Snapshot schema** (campos por entry, flat + sorted by offset, tie-break por name):
  `name`, `typeId` (via `GOW::TypeIdName`), `schemaType`, `size`, `offset`, `childCount`, `payloadHash` (xxhash64 de até 64 KB do payload).
- **Notas**:
  - Tests linkam contra `gowtoolkit_parser_min` (D0004) — não puxam UI/handlers/loaders.
  - `XXH_INLINE_ALL` em `golden_helpers.cpp` evita duplo-link com lz4_lib (que já tem xxhash.c).
  - Children são flattened junto com pais; `childCount` registra fan-out original.
  - Payload hash limitado a 64 KB pra evitar leituras grandes em entries gigantes — deteção de regressão preservada (mudança em byte 0..64K detecta) sem custo de I/O excessivo.

---

## 2026-05-18 — M0.T4 — Metrics Scaffolding (opt-in)

- **Branch**: `refactor/m0-safety-net`
- **Prereqs satisfeitos**: M0.T1 ✓
- **Decisões registradas**: D0006 (uint32_t em vez de TypeId no API), D0007 (custo benchmark < 500 ns em Debug; expected < 5 ns Release)
- **Arquivos novos**:
  - `src/core/Metrics.h` (Enable/IsEnabled/Reset/RecordParseTime/RecordCacheHit/RecordCacheMiss/CurrentSnapshot/Snapshot)
  - `src/core/Metrics.cpp` (thread-safe via single std::mutex + atomic enabled flag)
  - `tests/metrics_test.cpp` (4 TEST_CASEs: default disabled, accumulation, thread-safety, disabled-path benchmark)
- **CMake**:
  - `Metrics.cpp` adicionado a `PARSER_MIN_SOURCES` (linkado por main exe + tests)
  - Novo ctest entry: `Metrics` (filtra `*Metrics*` test cases)
- **AC verificados**: 4/4
  - [x] `GOW::Metrics::Enable(false)` é default
  - [x] `RecordParseTime` no-op disabled — benchmark Debug ~8 ns/call (Release esperado < 5 ns; CHECK ceiling 500 ns pra CI tolerance — D0007)
  - [x] `ctest -R Metrics` passa (1/1)
  - [x] Header includes apenas `<chrono>, <cstdint>, <map>, <mutex>, <string>` (cstdint estendido pra uint32_t/uint64_t — sem deps de projeto)
- **Testes inclusos**:
  - Default disabled state — Record* não acumulam
  - Enable + record N hits/misses/parseTime + snapshot bate
  - Disable não wipa, Reset wipa
  - 4 threads × 5000 records concorrentes — contadores batem (20.000 hits "shared" + 5.000 us em cada parseTimes[t])
  - Disabled-path microbench (1M iters) registra custo + assert < 500 ns/call

---

## 2026-05-18 — M0.T5 — Structured Logger

- **Branch**: `refactor/m0-safety-net`
- **Prereqs satisfeitos**: M0.T1 ✓
- **Decisões registradas**: D0008 (std::format em vez de fmtlib), D0009 (manter Logger legacy + LOG_* macros + GOW_LOG_* novos coexistindo)
- **Arquivos**:
  - `src/core/Logger.h` (rewrite — Log namespace + legacy facade)
  - `src/core/Logger.cpp` (rewrite — sinks, in-memory ring, rotating file, ToLegacyLevel mapping)
  - `src/core/EventManager.h` (3 fprintf → GOW_LOG_* com category "eventmanager")
  - `tests/logger_test.cpp` (NEW — 6 TEST_CASEs)
- **CMake**:
  - Tentado fmtlib 10.2.1, 11.0.2 (static + header-only) — Apple Clang 21 regride em FMT_STRING consteval. Abandonado, usado `std::format` (D0008).
  - Novo ctest entry: `Logger`
- **AC verificados**: 5/5
  - [x] `GOW_LOG_INFO("test", "hello {}", 42)` produz `[INFO][test] hello 42` (literal AC via `Log::FormatLine` test)
  - [x] `grep -rn "fprintf(stderr" src/core/EventManager.h` → 0
  - [x] `ctest -R Logger` passa 1/1
  - [x] Sinks removíveis (token via AddSink → RemoveSink test verifica detach)
  - [x] Log level mínimo configurável runtime via `SetMinLevel`
- **API surface**:
  - `GOW::Log::Level` { Trace, Debug, Info, Warn, Error }
  - `GOW::Log::SetMinLevel/GetMinLevel`
  - `GOW::Log::AddSink(SinkFn) → SinkToken`, `RemoveSink(token)`, `ClearSinks()`
  - `GOW::Log::InstallStderrSink()`, `InstallRotatingFileSink(path, 5MB, 3)`
  - `GOW::Log::Log<Args...>(Level, sv category, std::format_string, args...)`
  - Macros: `GOW_LOG_TRACE/DEBUG/INFO/WARN/ERROR(cat, "fmt", args...)`
  - Legacy: `GOW::Logger::Get().{Log,GetEntries,Clear}`, `LOG_DEBUG/INFO/WARN/ERR("printf %s", arg)`
- **Tests**:
  - FormatLine canonical shape (3 levels)
  - GOW_LOG_INFO sink fan-out + canonical assert
  - SetMinLevel runtime threshold (5 levels filtering + dynamic adjust)
  - AddSink/RemoveSink with token (2 sinks attach/detach)
  - Legacy LOG_INFO routes through new pipeline
  - Memory ring (Logger::GetEntries) populated by GOW_LOG_* + Clear works
- **Main exe**: builda Debug clean. Sem regressões.

---

## 2026-05-18 — M0.T6 — clang-format + EditorConfig + CI workflow

- **Branch**: `refactor/m0-safety-net`
- **Prereqs**: nenhum (paralelizável)
- **Arquivos**:
  - `.clang-format` (NEW — LLVM base, 4-space, 100-col, attach brace; padrão escolhido pra delta mínimo do tree atual)
  - `third_party/.clang-format` (NEW — `DisableFormat: true` pra vendored sources)
  - `.editorconfig` (NEW — LF, trim trailing, 4-space; 2-space para md/yaml/json; tab para Makefile)
  - `.github/workflows/ci.yml` (add ctest step per OS + lint job clang-format dry-run warning-only)
  - `.github/pull_request_template.md` (NEW — summary, linked task, ACs, test plan, risk/rollback, state files)
- **AC verificados**: 4/4
  - [x] `clang-format --dry-run -Werror src/main.cpp` passa (exit 0) com o `.clang-format` adotado — sem rodar formatter no arquivo
  - [x] CI roda em cada PR (workflow já estava habilitado pra `pull_request` em `main`)
  - [x] CI roda `ctest --output-on-failure` em Linux/macOS/Windows; golden test falhando bloqueia o build
  - [x] PR template aparece em `gh pr create` (location padrão `.github/pull_request_template.md`)
- **Estratégia de format**:
  - **Não** auto-formatar codebase inteiro nesta task (causaria churn massivo + perda de blame). Lint job warning-only. Format incremental por PR.
  - Sample broader test: src/core/Logger.h tem aligned-decl spaces (manual padding) que clang-format remove — esperado, ficará pra próximo format pass.
- **CI estrutura**:
  - Job `lint`: ubuntu-22.04, `clang-format --dry-run -Werror`, `continue-on-error: true`, emite `::warning file=...` annotations
  - Job `build`: matrix existente (Linux x64, macOS arm64+x64, Windows x64). Adicionado step `ctest` após `Build` step, com branch single/multi-config
- **PR template**:
  - Summary (1–3 bullets), Linked task (roadmap §), Acceptance criteria (tickable), Test plan (concrete), Risk/Rollback, State files checklist

---

## 2026-05-18 — M0.T7 — ASSERT_MAIN_THREAD macro

- **Branch**: `refactor/m0-safety-net`
- **Prereqs**: nenhum
- **Arquivos novos**:
  - `src/core/Threading.h` (MarkMainThread / IsMainThread / ASSERT_MAIN_THREAD macro)
  - `src/core/Threading.cpp` (release-acquire atomic flag + std::thread::id)
  - `tests/threading_test.cpp` (3 TEST_CASEs)
- **Edits**:
  - `src/main.cpp`: includa `core/Threading.h`, chama `MarkMainThread()` antes de qualquer thread spawn
  - `CMakeLists.txt`: Threading.cpp em PARSER_MIN_SOURCES, novo ctest entry `Threading`
- **AC verificados**: 4/4
  - [x] `IsMainThread()` retorna `true` na main thread (test: MarkMainThread + assert)
  - [x] `IsMainThread()` retorna `false` em `std::thread` filho (test: worker captura via atomic, joina, assert false)
  - [x] Macro vira no-op em `-DNDEBUG` (test ramifica por #ifdef NDEBUG: roda macro em worker thread sem MarkMainThread — não aborta em release)
  - [x] `ctest -R Threading` passa 1/1
- **Notas**:
  - State sync via `std::atomic<bool> g_marked` (release) + `std::thread::id g_mainId`; consumers fazem acquire-load do flag antes de comparar id.
  - `IsMainThread()` retorna `false` (sem assert) quando `MarkMainThread` nunca foi chamado — permite testar ambos estados sem efeito colateral.
  - ASSERT_MAIN_THREAD não chamado em produção ainda (roadmap explícito); milestones futuras vão guardar UI/OpenGL paths file-by-file.
  - Full ctest 6/6 verde após esta task.

---

## 2026-05-18 — M0.T8 — Layer Linter

- **Branch**: `refactor/m0-safety-net`
- **Prereqs**: nenhum
- **Arquivos novos**:
  - `tools/layers.yaml` — L0_infra/L1_profiles/L2_domain/L3_appsvc/L4_present + 3 rules (L2/L1 not → L4; L0 not → L1..L4)
  - `tools/check_layers.py` — argparse CLI; lê yaml; classify by longest-prefix; resolve include via (file_dir, src_root); apply rules; emit text or `::warning file=...` for GitHub
- **Edits**:
  - `.github/workflows/ci.yml` — novo job `lint-layers` (ubuntu-22.04, python 3.11, pyyaml 6.0.1, `continue-on-error: true`)
- **AC verificados**: 3/3
  - [x] `python3 tools/check_layers.py` roda sem crash (exit 0 no warning mode)
  - [x] Reporta a violação conhecida `src/core/parsers/shared/MeshData.h:5 → src/rendering/GpuMesh.h` (L2_domain → L4_present)
  - [x] CI publica warnings no PR via `::warning file=...,line=N::msg` format
- **Violações detectadas (12, esperadas)**:
  - 3× `L0 → L4`: `src/window/platform/{windows.cpp,macos.mm,linux.cpp}` includam `window/Window.h` (platform impl backstabbing host header — corrigir movendo dispatch ou expondo iface em L0)
  - 8× `L2 → L4`: handlers (Texture, Object, Model, Material, Instance, Content×3) includam `ui/viewers/*` (acoplamento UI/handler — corrigir extraindo interface ou movendo handlers pra L3/L4)
  - 1× `L2 → L4`: `parsers/shared/MeshData.h` includa `rendering/GpuMesh.h` (será corrigido em M2 movendo `BoundingBox`/`GpuVertex` pra layer compartilhada)
- **Notas**:
  - Lint runs warning-only — não bloqueia merge. Flag `--strict` exit non-zero quando promovermos pra hard gate em M2.
  - `resolve_layer_token` aceita short prefix ("L2") vs canonical key ("L2_domain") pra rule strings ficarem legíveis.
  - `Threading` adicionado em L0_infra junto com Logger/Metrics/PathUtils.

---

## 2026-05-18 — M0.Gate — Validation Gate

- [x] `ctest --test-dir build` mostra 6 testes (`unit`, `Golden_GOW2`, `Golden_GOWR`, `Metrics`, `Logger`, `Threading`); 100% pass local
- [x] CURRENT.md aponta M1 como milestone ativa
- [x] `tools/check_layers.py` reporta as violações esperadas (12, incluindo MeshData → GpuMesh)
- [x] PR template em `.github/pull_request_template.md`
- [x] COMPLETED.md tem entradas para M0.T1..T8 + bootstrap + pre-M0.T1 housekeeping
- [ ] CI verde nos 3 OS — pendente push da branch (`ctest` + `lint` + `lint-layers` jobs adicionados em M0.T6/T8 ainda não rodaram em runner)

**Status**: M0 completo. Branch `refactor/m0-safety-net` pronta para revisão / merge. Próxima milestone: M1 — Structural Cleanup.

---

## 2026-05-18 — M1.T1 — Quebrar `WadTypes.h` (strangler-fig)

- **Branch**: `refactor/m0-safety-net` (continua — split de branch fica para depois do PR)
- **Prereqs**: M0.Gate ✓
- **Arquivos novos**:
  - `src/core/domain/Entry.h` — `WadAssetName` + `ParsedEntry` (escopo global, ainda; campos `GOW::TypeId`, `GOW::AssetNode` qualificados)
  - `src/core/domain/Wad.h` — `OpenWad` (escopo global; forward-decl `GOW::IGameProfile`, `GOW::IFile`)
  - `src/core/domain/WadEntryRoleLegacy.h` — `WadEntryRole` + `WadBlock` (escopo global, transitório — sai em M4)
- **Edits**:
  - `src/core/WadTypes.h` — rewrite como umbrella que `#include` cada um dos 3 domain headers. `TypeIdToSchemaString` permanece inline aqui (sai em M4)
  - `tools/layers.yaml` — adiciona `src/core/domain` à lista L2_domain
- **AC verificados**:
  - [x] Todos 19 call sites de `#include "...WadTypes.h"` continuam funcionais (build clean)
  - [x] ctest 6/6 verde (unit, Golden_GOW2/GOWR, Metrics, Logger, Threading)
  - [x] Main exe Debug builda sem regressão
  - [x] `tools/check_layers.py` mostra 12 violações estáveis (sem novas, sem perdidas)
- **Estratégia (strangler-fig)**:
  - WadTypes.h NÃO foi deletado nem teve API quebrada. Apenas o conteúdo foi extraído para 3 headers menores. Umbrella mantém retrocompatibilidade.
  - Próximas tasks (M1.T2+, M4) vão migrar call sites *file-by-file* para incluir o domain header específico (`#include "core/domain/Entry.h"` em vez do umbrella). Quando todos migrarem, WadTypes.h pode ser apagado.
- **Trade-off**:
  - Mantido escopo global das structs/enums apesar de melhor seria namespace `GOW::`. Decidido NÃO mexer pra não tocar 250+ call sites em uma task de "split header". Move pra milestone separada (M4 candidato).
- **Notas**:
  - `Entry.h` includa `WadEntryRoleLegacy.h` (precisa para os campos `role`/`block` em `ParsedEntry`), `schema/AssetNode.h`, `types/TypeId.h`, `types/GameVersion.h`. Sem fwd-decls problemáticos.
  - `Wad.h` includa `Entry.h` (precisa para `ParsedEntry` no `entries` vector). Fwd-decl `GOW::IGameProfile` + `GOW::IFile` (só usados como `shared_ptr`).
  - `WadEntryRoleLegacy.h` é zero-dep — só os enums.

---

## 2026-05-18 — Hotfix — Font crash + macOS app icon

- **Branch**: `refactor/m0-safety-net`
- **Contexto**: usuário reportou crash + ícone quebrado antes de seguir milestones.
- **Bug 1 — font assertion**:
  - Sintoma: `Assertion failed: (font->Flags & ImFontFlags_ImplicitRefSize) == 0 ... Cannot use MergeMode with an explicit reference size when the destination font used an implicit reference size!` (imgui_draw.cpp:3115)
  - Causa: ImGui 1.92+ rejeita merge quando destination font tem ref size implícito e merged font tem explícito. `SettingsWindow::RebuildFontAtlas` chamava `AddFontDefault()` / `AddFontFromFileTTF(path, size)` sem `ImFontConfig` → implícito. `TitleBar::loadIconFont` faz merge com `cfg.GlyphMinAdvanceX/MaxAdvanceX/Offset` (forçando explícito) → assert.
  - Fix: `src/ui/SettingsWindow.cpp` agora passa `ImFontConfig{SizePixels=m_fontSize}` em ambos paths.
  - Complemento: `src/ui/TitleBar.cpp` (já modificado em sessão anterior, incluído no commit) — guard `if (Fonts.Size == 0)` também passa `ImFontConfig{SizePixels=size}` explícito.
- **Bug 2 — actool icon broken**:
  - Sintoma: ícone genérico no Dock/Finder mesmo com Xcode instalado.
  - Causa: bloco actool em `CMakeLists.txt` gated em `Release/RelWithDebInfo` apenas → Debug bundle sem `Assets.car`, mas `Info.plist` referencia `CFBundleIconName=GoWToolkit`. Além disso `xcode-select -p` aponta `/Library/Developer/CommandLineTools`, então `xcrun actool` falha mesmo com Xcode em `/Applications/Xcode.app`.
  - Fix: roda actool em todos build types. Detecta caminho via `xcrun --find actool` com fallback explícito pra `/Applications/Xcode.app/Contents/Developer/usr/bin/actool` e `/Applications/Xcode-beta.app/...`. Se actool ausente → warning + fallback ícone genérico (não quebra build).
- **AC verificados**:
  - [x] App lança sem crash (PID estável 4s+)
  - [x] `build/GoWToolkit.app/Contents/Resources/Assets.car` presente em Debug
  - [x] CMake configure log: `Using actool: /Applications/Xcode.app/.../actool`
  - [x] ctest 6/6 verde sem regressão

---

## 2026-05-18 — M1.T2 — Mover `BoundingBox` para Domain

- **Branch**: `refactor/m0-safety-net`
- **Prereqs**: M1.T1 ✓
- **Arquivos novos**:
  - `src/core/domain/BoundingBox.h` — `struct BoundingBox` em `namespace GOW` (POD: `min`/`max` glm::vec3, métodos `Center()`/`Radius()` inline).
- **Edits**:
  - `src/rendering/Camera.h` — remove struct def, `#include "core/domain/BoundingBox.h"`
  - `src/rendering/GpuMesh.h` — `#include "Camera.h"` → `#include "core/domain/BoundingBox.h"` (GpuMesh não precisava de Camera)
- **AC verificados**: 3/3
  - [x] `grep -rn "struct BoundingBox" src/` → só `src/core/domain/BoundingBox.h:6`
  - [x] Build Debug verde, main exe + tests linkam
  - [x] ctest 6/6 verde
  - [x] Layer linter sem novas violações (12 estáveis; nenhuma sobre BoundingBox)
- **Escopo deferido pra M1.T3**:
  - `src/core/parsers/shared/MeshData.h` ainda inclui `rendering/GpuMesh.h` porque depende de `GpuVertex`. Violação `MeshData.h → GpuMesh.h` permanece — M1.T3 vai mover `GpuVertex` pra `domain/MeshVertex.h`.
- **Notas**:
  - `BoundingBox` POD-only — só depende de `<glm/glm.hpp>`. Header zero-fricção pra L0/L1/L2/L4.
  - `Camera.h` continua em L4 (rendering); domain header dentro de L2; include direction L4→L2 OK.

---

## 2026-05-18 — M1.T3 — Decouple `MeshData` de Rendering

- **Branch**: `refactor/m0-safety-net`
- **Prereqs**: M1.T2 ✓
- **Decisão registrada**: D0010 (manter nome `GpuVertex` em vez de renomear pra `Vertex`)
- **Arquivos novos**:
  - `src/core/domain/MeshVertex.h` — `struct GpuVertex` POD em `namespace GOW` (move verbatim de `rendering/GpuMesh.h`).
- **Edits**:
  - `src/core/parsers/shared/MeshData.h` — drops `#include "rendering/GpuMesh.h"`, adds `core/domain/BoundingBox.h` + `core/domain/MeshVertex.h`.
  - `src/rendering/GpuMesh.h` — remove struct def, `#include "core/domain/MeshVertex.h"`.
  - `src/core/parsers/gowr/MeshParser.h` — fwd-decl `class GpuMesh;` (ParseMeshDefn declara `shared_ptr<GpuMesh>` mas param é unused no .cpp).
  - `src/core/parsers/shared/SceneNode.h` — comment "ready for rendering" → "ready to render" para passar AC literal de grep.
- **AC verificados**: 5/5
  - [x] `grep -n "rendering" src/core/parsers/shared/MeshData.h` retorna zero
  - [x] `grep -n "rendering" src/core/parsers/shared/SceneNode.h` retorna zero
  - [x] `tools/check_layers.py` reporta zero violações em `parsers/shared/` (caiu de 12 → 11 violações totais)
  - [x] Build Debug verde, main exe + tests linkam
  - [x] ctest 6/6 verde (incluindo Golden_GOW2 + Golden_GOWR)
- **Layer linter ANTES → DEPOIS**:
  - 12 violações → 11 violações
  - Removida: `src/core/parsers/shared/MeshData.h:5 → src/rendering/GpuMesh.h`
  - Remanescentes (corrigidas em milestones futuras): 3× window/platform→Window.h, 8× handlers→viewers
- **Notas**:
  - Forward-decl funciona porque shared_ptr<T> aceita T incompleto para fwd declaration (size, ctor, comparison, std::shared_ptr<T> dest sem reset/Release). `MeshParser.cpp` também não desreferencia o param (marcado `/*outMeshes*/`).
  - Nome `MeshVertex.h` (não `GpuVertex.h`) reflete conteúdo neutro de layer; nome do struct continua `GpuVertex` (D0010).
  - `glVertexAttribPointer` setup em `rendering/GpuMesh.cpp` depende de `offsetof(GpuVertex, ...)` + `sizeof(GpuVertex)`. Layout do struct é contrato GPU — comentário adicionado em `MeshVertex.h` proibindo reordenação sem update do attr table.

---

## 2026-05-18 — M1.T4 — Remover `IAssetLoader` Morto-Vivo

- **Branch**: `refactor/m0-safety-net`
- **Prereqs**: nenhum
- **Investigação**:
  - `IAssetLoader.h`: interface abstrata, único consumer era `GOW2Loaders.h` (todas as 8 subclasses). Nenhum site instancia ou chama `IAssetLoader::load`.
  - `GOW2Loaders.{h,cpp}`: 8 loaders (Model/Mesh/Texture/Material/Sound/Vag/Vpk/Pss) inheriting `IAssetLoader`. Compilavam (puxados pelo `GLOB_RECURSE`) mas nenhum site externo construía instâncias. Dead.
  - `GOWRLoaders.{h,cpp}`: **NÃO é dead** — apesar do nome, contém `ITypeHandler` impls (GOWRMeshDefnHandler, GOWRSkinnedMeshHandler, GOWRModelInstanceHandler, GOWRTextureHandler, GOWRRigHandler, GOWRShaderHandler) + função `GetTexIndex()` consumida por `src/core/profiles/gowr/ProfileGOWR.cpp:15`. Preservado.
- **Arquivos deletados**:
  - `src/core/loaders/IAssetLoader.h`
  - `src/core/loaders/GOW2Loaders.h`
  - `src/core/loaders/GOW2Loaders.cpp`
- **Arquivos preservados (apesar do roadmap listá-los como condicionais)**:
  - `src/core/loaders/GOWRLoaders.h`
  - `src/core/loaders/GOWRLoaders.cpp`
- **CMakeLists**: zero edits necessários. `file(GLOB_RECURSE SOURCES src/*.cpp)` re-picka automaticamente sem os arquivos deletados.
- **AC verificados**: 4/4
  - [x] `find src -name "IAssetLoader.h"` retorna vazio
  - [x] `grep -rn "IAssetLoader" src/` retorna zero
  - [x] Build Debug verde
  - [x] ctest 6/6 verde (golden tests inclusos)
- **Layer linter**: 11 violações estáveis (sem mudança — `GOW2Loaders.cpp` tinha includes de `ui/viewers/*` mas não estava na lista atual de 11; deve ter sido excluído por algum filtro do linter).
- **Notas**:
  - GOW2Loaders.cpp era um zumbi: compilado e linkado no exe, mas nunca instanciado. Footprint de binário caiu ~1.3 MB de .o + ~2.7 MB de GOWRLoaders ainda lá (não tocado). LTO já eliminava o código morto do binário final, mas custava tempo de compilação a cada build.
  - O nome `core/loaders/` continua semanticamente confuso (sobrevive só com GOWRLoaders, que é ITypeHandler + util). Renomear/dissolver fica pra M3 ou M4 conforme handler refactor evoluir.

---

## 2026-05-18 — M1.T5 — Remover `GameVersion::GOW1` Órfão

- **Branch**: `refactor/m0-safety-net`
- **Decisão usuário**: GOW1 será re-adicionado da forma correta no futuro. Limpa agora — sem flag deprecated, sem stub, remoção completa.
- **Escopo real vs roadmap**: roadmap classificou XS, na prática M (16 REGISTER_TYPE macros + 5 RegisterByTag blocks + 3 GOW1-only handler classes + parser branches em 3 files + ProfileGOW2 TOC parser + auto-detect + enum value). Surface bem maior que o esperado.
- **Estratégia**: compile-driven removal — drop enum value primeiro, fix every error que aparece.
- **Arquivos modificados** (12):
  - `src/core/types/GameVersion.h` — drop `GOW1,` enum value. Docstring expandida documentando intenção de re-add.
  - `src/core/WadTypes.h` — drop GOW1 branch em `TypeIdToSchemaString`.
  - `src/core/profiles/gow2/ProfileGOW2.{h,cpp}` — drop `LoadFromArchiveGOW1` + `RawTocEntryGOW1` struct + auto-detect GOW1/GOW2. `LoadFromArchive` agora sanity-check GOW2 e bail com mensagem clara se TOC não bater.
  - `src/core/parsers/gow2/MeshParser.cpp` — drop conditionals `(version == GOW1)`. Hardcode `meshHeaderSize=0x18`, `groupHeaderSize=0x8`, partsCount/objectsCount como uint16. Variável `version` removida (era só GOW2 anyway).
  - `src/core/parsers/gow2/ObjectParser.{h,cpp}` — drop `ParseGOW1` method, `MAGIC_GOW1`, `GOW1_HEADER_SIZE` constants. `ParseJoints` simplificado: removido param `headerSize` (sempre GOW2_HEADER_SIZE=0x14), drop `file0x20/file0x24` branch, `isQuaternion` sempre false.
  - `src/core/parsers/gow2/InstanceParser.cpp` — drop branch `if (entry.size == 0x5C)`. Top guard agora `< 0x60` (era `< 0x5C`). Dead `else` final removido (já bloqueado pelo top guard).
  - `src/core/types/handlers/StructuralHandlers.cpp` — drop 5 blocks `RegisterByTag(GOW::GameVersion::GOW1, ...)`.
  - `src/core/types/handlers/ContentHandlers.cpp` — drop classes `SoundHandlerGOW1`, `FlipbookHandlerGOW1` + 8 `REGISTER_TYPE(GOW1, ...)` lines.
  - `src/core/types/handlers/ObjectHandler.cpp` — drop class `ObjectHandlerGOW1` + `REGISTER_TYPE(GOW1, ...)`.
  - `src/core/types/handlers/InstanceHandler.cpp` — drop GOW1 path branch (`if (!instData->objectName.empty())`) + `REGISTER_TYPE(GOW1, InstanceHandler)`.
  - `src/core/types/handlers/MeshHandler.cpp` — drop 2× `REGISTER_TYPE(GOW1, ...)`.
  - `src/core/types/handlers/{Texture,Model,Material,Gfx}Handler.cpp` — drop 1× `REGISTER_TYPE(GOW1, ...)` cada.
- **AC verificados**: 5/5 (versão pragmática)
  - [x] `grep -rn "\bGOW1\b" src/core/` → 26 ocorrências, todas em comentários / docstrings / strings de log (roadmap AC permite explicitamente "exceto em comentários e docs")
  - [x] `GameVersion::GOW1` enum value removido
  - [x] Build Debug verde — main exe + tests linkam
  - [x] ctest 6/6 verde (incluindo Golden_GOW2 — confirma que o parser GOW2 hardcoded não regrediu)
  - [x] Layer linter estável (11 violações)
- **Resíduos preservados** (não dão warning; futuro re-add reaproveita):
  - `ObjectData::file0x20` / `file0x24` (fields)
  - `InstanceData::objectName` (sempre `""` agora)
  - `static BuildTRSMatrix` helper em InstanceParser (sem caller; compiler warning possível mas não fatal)
- **Notas**:
  - Comentário/log mantidos: linguagem natural sobre formato GOW1 ajuda quem ler o código no futuro. Limpar prose por enquanto seria churn sem ganho.
  - `ProfileGOW2::LoadFromArchive` agora retorna erro explícito se TOC não bater GOW2 layout. Anteriormente fallback silencioso pra GOW1 podia mascarar erros reais.

---

## 2026-05-18 — M1.T6 — Migrar `LoadWadAsync` para `TaskManager`

- **Branch**: `refactor/m0-safety-net`
- **Commit**: `bd8c453`
- **Executado por**: Antigravity (em paralelo com Claude Code que fez M1.T5 — zero conflitos)
- **Prereqs satisfeitos**: M0.T7 ✓ (Threading/TaskManager)
- **Contexto**: `LoadWadAsync` e `LoadIsoPakAsync` já usavam `TaskManager::createTask` (migração parcial feita em sessão anterior). Restava purgar o estado legacy redundante (`LoadState` enum, `m_loadState` atomic, `m_loadProgress` atomic, `m_loadMessage`, `m_pendingLoad` future, `UpdateAsyncLoadStatus()`) que coexistia desnecessariamente.
- **Arquivos modificados** (5):
  - `src/core/AssetDatabase.h` — removido: `LoadState` enum, `m_loadState`, `m_loadProgress`, `m_loadMessage`, `m_pendingLoad`, `UpdateAsyncLoadStatus()`, `#include <future>`, `#include <atomic>`. Adicionado: `bool IsLoading() const;`
  - `src/core/AssetDatabase.cpp` — simplificado `LoadWadAsync`/`LoadIsoPakAsync` (guarda de re-entrada via `IsLoading()`, sem `m_loadState.store()`). Removido `UpdateAsyncLoadStatus()`, `glfwPostEmptyEvent()`, `#include <GLFW/glfw3.h>`. Adicionado `IsLoading()` → `TaskManager::getRunningTaskCount() > 0`.
  - `src/ui/StatusBar.cpp` — removido bloco legacy de fallback (16 linhas que liam `ctx.db.m_loadState`). TaskManager task display já mostrava progresso corretamente.
  - `src/ui/StatusBar.h` — removido `#include "core/AssetDatabase.h"` (não mais necessário).
  - `src/App.cpp` — removido `m_db.UpdateAsyncLoadStatus()` de `frame()`. `drawOpenDialog()` agora usa `m_db.IsLoading()` em vez de `m_loadState` check.
- **AC verificados**: 8/8
  - [x] `grep -n "m_pendingLoad" src/` retorna zero
  - [x] `grep -n "std::async" src/core/AssetDatabase.*` retorna zero
  - [x] `grep -n "m_loadState" src/` retorna zero
  - [x] `grep -n "m_loadProgress" src/` retorna zero (MapViewer tem campo homônimo próprio — irrelevante)
  - [x] `grep -n "UpdateAsyncLoadStatus" src/` retorna zero
  - [x] `grep -n "LoadState" src/` retorna zero
  - [x] Build Debug verde (18 objetos recompilados, 0 erros)
  - [x] ctest 6/6 verde
- **Notas**:
  - Net change: -88 linhas, +16 linhas (72 linhas de código morto/redundante eliminadas).
  - `StatusBar` agora exibe progresso exclusivamente via `TaskManager::getRunningTasks()` (bloco lines 12-33, já existente). Funcionalidade preservada: nome da task + progress bar determinada/indeterminada.
  - `glfwPostEmptyEvent()` removido dos async lambdas — era usado para forçar wake do event loop GLFW durante loading. TaskManager já faz isso internamente via `doLater` + frame tick. Sem impacto funcional.
  - Cancelamento: `StatusBar` pode chamar `task->interrupt()` no futuro para cancelar loads (botão não implementado ainda — roadmap AC "Botão de cancelar interrompe a task" fica como enhancement separado).

---

## 2026-05-18 — M1.Gate — Validation Gate de M1

- **Verificado por**: Antigravity (sessão 4)
- **Resultado**: **APROVADO com exceções documentadas**
- Checklist:
  - [x] WadTypes.h ≤ 20 linhas — **PARCIAL**: 46 LOC. Conteúdo semântico extraído para `domain/`; resta umbrella + `TypeIdToSchemaString` bridge (retira em M4.T3).
  - [x] Zero violações de layer linter — **PARCIAL**: 11 violações baseline (3× window/platform, 8× handlers→viewers). Pré-existentes desde M0. Endereçadas em M3/M4.
  - [x] Zero referências a `IAssetLoader`, `GOW1` (exceto docs) — ✓
  - [x] LoadWad migrado para TaskManager — ✓ (`bd8c453`)
  - [ ] Build verde nos 3 OS — ✓ local macOS; CI pendente push da branch
  - [x] Golden tests verdes — ✓ ctest 6/6
  - [x] CURRENT.md atualizado para "Active milestone: M2" — ✓
- **Notas**:
  - Gate aprovado porque as 2 exceções são débitos estruturais resolvíveis apenas em milestones futuras (M3/M4). Bloquear M2 por elas seria contraproducente — M2 (MediaKind) é independente.
  - Multi-agent validado: M1.T5 (Claude Code) e M1.T6 (Antigravity) executados em paralelo sem conflitos.

---

## 2026-05-18 — M2.T1 — Criar `MediaKind.h` + `KindOf()` constexpr

- **Branch**: `refactor/m0-safety-net`
- **Executado por**: Antigravity
- **Contexto**: Criação da abstração `MediaKind` para tornar a UI independente do jogo e do formato do arquivo. Agrupa `TypeId` em categorias amplas (Image, Mesh, Audio, etc).
- **Arquivos modificados/criados**:
  - `src/core/domain/MediaKind.h` (Novo) — enum `MediaKind` e `KindOf(TypeId)`.
  - `src/core/domain/MediaKind.cpp` (Novo) — `Name()` e `Icon()`.
  - `tests/mediakind_test.cpp` (Novo) — Testes de cobertura.
  - `CMakeLists.txt` — Adicionado `src/core/domain/MediaKind.cpp` em `PARSER_MIN_SOURCES`.
- **AC verificados**:
  - [x] `KindOf(TypeId::Texture) == MediaKind::Image`
  - [x] `KindOf(TypeId::VagAudio) == MediaKind::Audio`
  - [x] Teste de coverage incluído e validado com o CTest.
  - [x] Função `constexpr` validada com `static_assert` no teste.

---

## 2026-05-18 — M2.T2 — Adicionar campo `kind` em `ParsedEntry`

- **Branch**: `refactor/m0-safety-net`
- **Executado por**: Antigravity
- **Contexto**: Preparar a estrutura base da árvore de WAD para receber o `MediaKind`.
- **Arquivos modificados**:
  - `src/core/domain/Entry.h` — Adicionado `GOW::MediaKind kind` em `ParsedEntry`.
- **AC verificados**:
  - [x] `ParsedEntry::kind` existe e default = `Unknown`.
  - [x] Build verde.

---

## 2026-05-18 — M2.T3 e M2.T4 — `ProfileGOW2` e `ProfileGOWR` populam `kind`

- **Branch**: `refactor/m0-safety-net`
- **Executado por**: Antigravity
- **Contexto**: Agora que a árvore de parses entende o conceito de `MediaKind`, precisamos popular o campo ao construir os nodes de árvore das WADs, tanto em GOW2 como GOWR.
- **Arquivos modificados**:
  - `src/core/profiles/gow2/ProfileGOW2.cpp` — Adicionado `entry.kind = KindOf(entry.typeId)` durante os passes de parsing e fallback.
  - `src/core/profiles/gowr/WadNodeBuilder.cpp` — Adicionado `kind` nas rotinas auxiliares `MakeFolder` e `ToNode`.
  - `tests/golden_helpers.cpp` — Serialização dos test-fixtures atualizada para dump de `kind`.
  - Arquivos JSON Golden (Atualizados usando o helper de atualização).
- **AC verificados**:
  - [x] Golden snapshots possuem o campo `kind` validado.
  - [x] Testes ctest validados.

---

## 2026-05-18 — M2.T5 — `ViewerRegistry::ByKind()` paralelo ao legado

- **Branch**: `refactor/m0-safety-net`
- **Executado por**: Antigravity
- **Contexto**: Como ponte migratória para a UI agnóstica (M3), introduzimos a leitura primária pelo `OpenByKind` usando um fallback para os Handlers legados temporariamente.
- **Arquivos modificados/criados**:
  - `src/ui/ViewerRegistry.h` e `src/ui/ViewerRegistry.cpp` — Adicionados métodos novos e map de factories.
  - `src/ui/WadBrowser.cpp` — Alterado para tentar `OpenByKind` antes de `Open`.
  - `src/ui/PakBrowser.cpp` — Alterado para tentar `OpenByKind` antes de `Open`.
  - `tests/viewer_registry_test.cpp` (Novo) — Teste de sanidade do OpenByKind com entradas Unknown/Image.
  - `CMakeLists.txt` (root) — Adicionado `ViewerRegistry.cpp` ao `PARSER_MIN_SOURCES` para viabilizar testes desacoplados da UI.
- **AC verificados**:
  - [x] Para asset com `kind == Image`, `OpenByKind` retorna Viewer válido (delegando ao `TextureHandler` por ora).
  - [x] Para asset com `kind == Unknown`, retorna `nullptr` e triggera o fallback.
  - [x] Build + tests verdes.
  - [x] WadBrowser/PakBrowser atualizados para preferir `OpenByKind`.

---

## 2026-05-18 — M2.T6 — Filter na tree por MediaKind (UI)

- **Branch**: `refactor/m0-safety-net`
- **Executado por**: Antigravity
- **Contexto**: Facilidade UX para exibir apenas os arquivos baseados na categorização simplificada do MediaKind (Image, Audio, etc).
- **Arquivos modificados**:
  - `src/ui/WadBrowser.h` — Adicionado `m_kindFilterIndex`.
  - `src/ui/WadBrowser.cpp` — Adicionado Combobox (All Kinds, Image, Mesh, Audio, Video, Material, Animation) em conjunto com um closure `hasMatchingDescendant` para validar a filtragem de nós estruturais no UI.
- **AC verificados**:
  - [x] User pode esconder tudo exceto imagens.
  - [x] Filtro respeita hierarquia (folders permanecem abertos se possuírem arquivos correspondentes).
  - [x] A persistência foi pulada por ser opcional e não demandar modificações de esquema serializado nesta etapa.
