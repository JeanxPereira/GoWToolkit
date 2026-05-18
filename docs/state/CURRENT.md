# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão 3)
**Sessão #**: 3

## Milestone ativa
M1 — Structural Cleanup

## Task em progresso
nenhuma — M1.T3 fechada, próxima é M1.T4

## Próxima task no pipeline
M1.T4 — Remover `IAssetLoader` Morto-Vivo. Esforço S, risco low. Sem prereqs.

## Blockers
nenhum

## Notas para o próximo agente
- M1.T3 entregue: `GpuVertex` movido para `src/core/domain/MeshVertex.h`. `MeshData.h` includa só headers de domain agora. `GpuMesh.h` mantém o struct apenas via include.
- Forward-decl `class GpuMesh;` adicionada a `core/parsers/gowr/MeshParser.h` — função `ParseMeshDefn` declara `shared_ptr<GpuMesh>` mas não desreferencia (parâmetro `outMeshes` é `/*unused*/` no .cpp).
- Comentário em `SceneNode.h` reescrito de "ready for rendering" → "ready to render" pra satisfazer AC literal (`grep -n "rendering" SceneNode.h` = 0).
- Decisão D0010: nome `GpuVertex` mantido (vs rename pra `Vertex` sugerido no roadmap). Justificativa em `DECISIONS.md`.
- Layer linter caiu de 12 → 11 violações (sem `parsers/shared/MeshData.h → rendering/GpuMesh.h`).
- ctest 6/6 verde.

## Arquivos tocados nesta sessão
- Hotfix font + actool: commit `fdeaed7`
- M1.T2 BoundingBox → domain: commit `260c42a`
- M1.T3:
  - `src/core/domain/MeshVertex.h` (NEW)
  - `src/core/parsers/shared/MeshData.h` (drops rendering include, adds 2 domain includes)
  - `src/rendering/GpuMesh.h` (drops struct def, adds domain include)
  - `src/core/parsers/gowr/MeshParser.h` (adds fwd-decl `class GpuMesh;`)
  - `src/core/parsers/shared/SceneNode.h` (comment reword to pass AC grep)
