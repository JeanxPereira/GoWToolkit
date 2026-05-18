# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão 3)
**Sessão #**: 3

## Milestone ativa
M1 — Structural Cleanup

## Task em progresso
nenhuma — M1.T4 fechada, próxima é M1.T5

## Próxima task no pipeline
M1.T5 — Limpar `GameVersion::GOW1` Órfão. Esforço XS, risco low. Sem prereqs.

## Blockers
nenhum

## Notas para o próximo agente
- M1.T4 entregue: deletado `IAssetLoader.h`, `GOW2Loaders.h`, `GOW2Loaders.cpp` (todos confirmados dead).
- `GOWRLoaders.{h,cpp}` PRESERVADO (não é IAssetLoader-derived; contém `ITypeHandler` impls + `GetTexIndex()` consumido por `ProfileGOWR.cpp:15`). Roadmap listava sua deleção como condicional ("se confirmado dead").
- CMakeLists não precisou de edit — `file(GLOB_RECURSE SOURCES src/*.cpp)` re-picka automaticamente.
- Layer linter estável em 11 violações.
- ctest 6/6 verde.

## Progresso M1
- T1 ✓ commit `6454eac`
- T2 ✓ commit `260c42a`
- T3 ✓ commit `8b0e7f2`
- T4 ✓ (commit pendente nesta sessão)
- T5, T6, Gate restantes

## Arquivos tocados nesta sessão (cumulativo)
- Hotfix font + actool: commit `fdeaed7`
- M1.T2 BoundingBox → domain: commit `260c42a`
- M1.T3 MeshData decouple: commit `8b0e7f2`
- M1.T4 IAssetLoader+GOW2Loaders dead-code purge:
  - DELETED `src/core/loaders/IAssetLoader.h`
  - DELETED `src/core/loaders/GOW2Loaders.h`
  - DELETED `src/core/loaders/GOW2Loaders.cpp`
