# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão de continuação)
**Sessão #**: 2

## Milestone ativa
M0 — Safety Net

## Task em progresso
nenhuma — M0.T7 fechada, próxima é M0.T8 (última do M0)

## Próxima task no pipeline
M0.T8 — Layer Linter
- **Prereqs**: nenhum
- Arquivos: `tools/check_layers.py`, `tools/layers.yaml`, `.github/workflows/ci.yml` (adicionar job)
- Hierarquia L0..L4 definida em §M0.T8 do roadmap
- Inicialmente **warning** (M2 transforma em failure)
- AC: script roda sem crash + lista pelo menos a violação `parsers/shared/MeshData.h → rendering/GpuMesh.h`

## Blockers
nenhum

## Notas para o próximo agente
- `GOW::Threading::MarkMainThread()` é chamado no início de `main()` antes de `registerProfiles`. Qualquer thread spawnada depois disso verá `IsMainThread() == false`.
- `ASSERT_MAIN_THREAD()` macro é no-op em `NDEBUG` (release). Não está chamado em call sites de produção ainda — M0.T7 só wirou a primitiva.
- Threading.cpp está no `PARSER_MIN_SOURCES` (mesmo que parser não use — eu adicionei pra test binary linkar; main exe também).
- M0.T8 vai ler `#include` strings de `src/**/*.{h,cpp}` (skip third_party + build) e bater contra `layers.yaml`. Considerar usar `pathlib` + regex `^\s*#include\s+["<]([^">]+)[">]`.
- Violação garantida pra detectar (AC): `src/core/parsers/shared/MeshData.h` includa `rendering/GpuMesh.h` — já confirmado no grep da seção M0.T3.
- Roadmap completo em `docs/ROADMAP_IMPLEMENTATION.md` §M0.T8 + §M0.Gate (validation gate de M0 após T8 fechar).

## Arquivos tocados nesta sessão (acumulado)
### M0.T1–T6: ver entries anteriores

### M0.T7
- `src/core/Threading.{h,cpp}` (NEW — MarkMainThread, IsMainThread, ASSERT_MAIN_THREAD)
- `src/main.cpp` (`#include "core/Threading.h"` + `MarkMainThread()` call)
- `CMakeLists.txt` (Threading.cpp em PARSER_MIN_SOURCES, Threading ctest entry)
- `tests/threading_test.cpp` (NEW — 3 TEST_CASEs: main affinity, worker thread != main, NDEBUG/debug macro behavior)
