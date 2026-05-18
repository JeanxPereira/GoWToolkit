# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão de continuação)
**Sessão #**: 2

## Milestone ativa
M1 — Structural Cleanup

## Task em progresso
nenhuma — M1.T1 fechada, próxima é M1.T2

## Próxima task no pipeline
M1.T2 — verificar §M1.T2 do roadmap. Sem dependência declarada.

## Blockers
nenhum

## Notas para o próximo agente
- M1.T1 entregue via strangler-fig: `WadTypes.h` virou umbrella include que pulla `domain/Entry.h`, `domain/Wad.h`, `domain/WadEntryRoleLegacy.h`. Todos os 19 call sites continuam compilando sem mudança.
- `TypeIdToSchemaString` ficou em `WadTypes.h` (sai em M4 quando schemaString morre).
- `WadAssetName`, `ParsedEntry`, `OpenWad`, `WadEntryRole`, `WadBlock` continuam em **escopo global** (compat com legacy). Migrar pra `GOW::` namespace em milestone futura.
- `tools/layers.yaml` ganhou `src/core/domain` em L2_domain. Run `python3 tools/check_layers.py` ainda reporta 12 violações conhecidas (sem regressão).
- ctest 6/6 verde após split.
- Roadmap §M1.T2 e depois — ver `docs/ROADMAP_IMPLEMENTATION.md`.

## Arquivos tocados nesta sessão (M0 + M1.T1)
- M0.T1–T8: ver entries anteriores
- M1.T1:
  - `src/core/domain/Entry.h` (NEW — WadAssetName + ParsedEntry)
  - `src/core/domain/Wad.h` (NEW — OpenWad)
  - `src/core/domain/WadEntryRoleLegacy.h` (NEW — WadEntryRole + WadBlock; sai em M4)
  - `src/core/WadTypes.h` (rewrite — umbrella include + TypeIdToSchemaString)
  - `tools/layers.yaml` (add `src/core/domain` em L2)
