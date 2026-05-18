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
