# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão de continuação)
**Sessão #**: 2

## Milestone ativa
M0 — Safety Net

## Task em progresso
nenhuma — M0.T3 fechada, próxima é M0.T4

## Próxima task no pipeline
M0.T4 — Metrics Scaffolding (opt-in)
- **Prereqs satisfeitos**: M0.T1 ✓
- Arquivos esperados: `src/core/Metrics.h`, `src/core/Metrics.cpp`, `tests/metrics_test.cpp`
- Independente de M0.T2/M0.T3 (paraleliza com M0.T5)

## Blockers
nenhum

## Notas para o próximo agente
- `gowtoolkit_parser_min` (D0004) é a unit de linkage pra qualquer test que precise rodar parser real. Lista de sources hardcoded em `CMakeLists.txt`. Adicionar parser novo em `src/core/profiles/` → atualizar `PARSER_MIN_SOURCES` no CMakeLists.
- M0.T4 (Metrics) deve viver no core e ser opt-in (`#ifdef GOWTOOLKIT_ENABLE_METRICS` ou flag CMake). Se entrar no parser-min, manter PARSER_MIN_SOURCES sincronizado.
- `tools/regenerate_goldens.sh` é o gateway pra atualizar `*.expected.json` quando parser muda intencionalmente. NUNCA editar o JSON manualmente.
- Hash do snapshot é xxhash64, NÃO SHA-1 (D0005). Tem xxhash.h via lz4_lib.
- Test infra usa `GOWTOOLKIT_TEST_FIXTURES_DIR` (definição CMake) pra resolver path independente de CWD.
- Mutation test pattern: editar parser, `ninja -C build gowtoolkit_tests`, `ctest --test-dir build -R Golden` — esperar fail com diff. Reverter, reusar.
- Roadmap completo em `docs/ROADMAP_IMPLEMENTATION.md` §M0.T4.

## Arquivos tocados nesta sessão
### Pre-M0.T1 housekeeping
- `docs/state/{CURRENT,COMPLETED,DECISIONS}.md` (state scaffold)
- `docs/{FORMAT_TEMPLATE,GoW1/Formats/*,GoW2/Formats/*,GoWRknk/*}.md` (reorg)

### M0.T1
- `CMakeLists.txt` (doctest FetchContent, CTest, add_subdirectory(tests))
- `tests/CMakeLists.txt`, `tests/main.cpp`, `tests/sanity_test.cpp`

### M0.T2
- `tools/make_test_fixtures.py`
- `tests/fixtures/{gow2,gowr}/wad_minimal.wad`
- `tests/fixtures/README.md`
- `.gitattributes`

### M0.T3
- `CMakeLists.txt` (gowtoolkit_parser_min lib, nlohmann/json FetchContent, separate Golden_* ctest entries)
- `tests/CMakeLists.txt` (links parser-min + nlohmann_json + fixtures dir define)
- `tests/test_stubs.cpp` (GetTexIndex stub)
- `tests/golden_helpers.{h,cpp}` (SnapshotEntries / LoadGolden / DiffSnapshots / RunGoldenTest)
- `tests/golden_gow2.cpp`, `tests/golden_gowr.cpp`
- `tests/fixtures/{gow2,gowr}/wad_minimal.expected.json`
- `tools/regenerate_goldens.sh`
