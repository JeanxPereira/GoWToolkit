# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão de continuação)
**Sessão #**: 2

## Milestone ativa
M0 — Safety Net

## Task em progresso
nenhuma — M0.T4 fechada, próxima é M0.T5

## Próxima task no pipeline
M0.T5 — Structured Logger
- **Prereqs satisfeitos**: M0.T1 ✓
- Reescrita de `src/core/Logger.{h,cpp}` com fmtlib, levels, sinks
- Vai conflitar com versão atual de Logger.cpp (já no parser-min). Precisa atualizar incidência em EventManager.h (AC explícito).

## Blockers
nenhum

## Notas para o próximo agente
- `Metrics.h` é leaf — pode ser dropado em qualquer TU sem deps de projeto. Includes: `<chrono>, <cstdint>, <map>, <mutex>, <string>`.
- API de Metrics aceita `uint32_t typeId` (D0006); caller faz `static_cast<uint32_t>(GOW::TypeId::...)`.
- `Metrics::Enable(false)` é default; hot path disabled custa ~8 ns Debug, < 1 ns Release esperado (D0007).
- `Metrics.cpp` está no `gowtoolkit_parser_min` (CMakeLists.txt PARSER_MIN_SOURCES). Adicionar Logger refactor (M0.T5) idem.
- M0.T5 vai trazer fmtlib via FetchContent — verificar `CMAKE_POLICY_VERSION_MINIMUM` workaround se precisar (mesmo padrão do doctest).
- AC M0.T5: substituir `fprintf(stderr, ...)` em `src/core/EventManager.h` (escopo limitado). NÃO refatorar Logger em todos call sites — só o EventManager.
- Roadmap completo em `docs/ROADMAP_IMPLEMENTATION.md` §M0.T5.

## Arquivos tocados nesta sessão (acumulado)
### M0.T1
- `CMakeLists.txt` (doctest + CTest)
- `tests/{CMakeLists.txt, main.cpp, sanity_test.cpp}`

### M0.T2
- `tools/make_test_fixtures.py`
- `tests/fixtures/{gow2,gowr}/wad_minimal.wad`
- `tests/fixtures/README.md`
- `.gitattributes`

### M0.T3
- `CMakeLists.txt` (gowtoolkit_parser_min lib, nlohmann/json, Golden_* ctest entries)
- `tests/{CMakeLists.txt, test_stubs.cpp, golden_helpers.{h,cpp}, golden_gow2.cpp, golden_gowr.cpp}`
- `tests/fixtures/{gow2,gowr}/wad_minimal.expected.json`
- `tools/regenerate_goldens.sh`

### M0.T4
- `src/core/Metrics.{h,cpp}` (NEW)
- `tests/metrics_test.cpp` (NEW)
- `CMakeLists.txt` (Metrics.cpp em PARSER_MIN_SOURCES, Metrics ctest entry)
