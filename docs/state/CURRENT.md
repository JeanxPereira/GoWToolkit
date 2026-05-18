# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão de continuação)
**Sessão #**: 2

## Milestone ativa
M1 — Structural Cleanup (M0.Gate passou)

## Task em progresso
nenhuma — esperando próxima sessão começar M1.T1

## Próxima task no pipeline
M1.T1 — Quebrar `WadTypes.h`
- **Prereqs satisfeitos**: M0.Gate ✓
- Strangler-fig: `WadTypes.h` continua existindo como umbrella include
- Novos arquivos: `src/core/domain/Entry.h`, `src/core/domain/Wad.h`, `src/core/domain/WadEntryRoleLegacy.h`
- AC: todos `#include "core/WadTypes.h"` continuam funcionais; build limpa

## M0.Gate — Validation Gate de M0 (passou)
- [x] `ctest --test-dir build` mostra ≥ 5 testes: tem 6 entries (`unit` + `Golden_GOW2` + `Golden_GOWR` + `Metrics` + `Logger` + `Threading`). Local: 6/6 green.
- [ ] CI verde nos 3 OS — **não verificado localmente; CI vai rodar no push**. Workflow editado em M0.T6, lint:layers + ctest steps adicionados.
- [x] `CURRENT.md` declara "M1" como milestone ativa (este arquivo)
- [x] `tools/check_layers.py` roda + reporta 12 violações conhecidas (incluindo MeshData → GpuMesh)
- [x] PR template configurado (`.github/pull_request_template.md`)
- [x] COMPLETED.md tem 8 entradas M0.T1..T8 + 2 bootstrap/housekeeping

## Blockers
- CI green nos 3 OS não verificado localmente. Push da branch dispara workflow. Se Apple Clang 21 no runner macos-14 reproduzir o erro local de `actool` em Release, vai precisar fix separado. Linux + Windows não tem actool — provavelmente OK.

## Notas para o próximo agente
- M0 fechado. Branch `refactor/m0-safety-net` pronta pra PR para `main`.
- Sugestão: abrir PR usando `.github/pull_request_template.md` (preenche linked task = M0.* + ACs ticked). Ou criar branch `refactor/m1-structural` baseada em main após M0 mergear.
- Layer linter detecta 12 violações conhecidas — listadas em COMPLETED.md M0.T8 entry. Não corrigir agora (M2+ tarefa).
- M1.T1 quebra WadTypes.h via strangler-fig: NÃO remover WadTypes.h. Cria headers novos em `src/core/domain/` e WadTypes.h umbrella include them. Build deve compilar sem mudar nenhum call site.
- M1.T1 vai mexer em `PARSER_MIN_SOURCES` se mover sources (improvável — só headers). Verificar ctest 6/6 após mudanças.
- Roadmap completo em `docs/ROADMAP_IMPLEMENTATION.md` §M1.T1.

## Arquivos tocados nesta sessão (M0 acumulado)
- M0.T1: doctest scaffold (CMakeLists, tests/{CMakeLists,main,sanity})
- M0.T2: WAD fixtures + truncator (.gitattributes, tools/make_test_fixtures.py, tests/fixtures/{gow2,gowr}/wad_minimal.wad, README)
- M0.T3: golden runner (parser-min lib, nlohmann/json, tests/golden_*, *.expected.json, tools/regenerate_goldens.sh, tests/test_stubs.cpp)
- M0.T4: Metrics (src/core/Metrics.{h,cpp}, tests/metrics_test.cpp)
- M0.T5: Logger rewrite (src/core/Logger.{h,cpp}, src/core/EventManager.h fprintf migration, tests/logger_test.cpp)
- M0.T6: clang-format + editorconfig + CI tests + lint (.clang-format, third_party/.clang-format, .editorconfig, .github/workflows/ci.yml, .github/pull_request_template.md)
- M0.T7: Threading (src/core/Threading.{h,cpp}, src/main.cpp, tests/threading_test.cpp)
- M0.T8: layer linter (tools/check_layers.py, tools/layers.yaml, CI lint-layers job)
