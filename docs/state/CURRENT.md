# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão 3)
**Sessão #**: 3

## Milestone ativa
M1 — Structural Cleanup

## Task em progresso
nenhuma — M1.T5 fechada, próxima é M1.T6

## Próxima task no pipeline
M1.T6 — Migrar `LoadWadAsync` para `TaskManager`. Esforço M, risco med. Prereq M0.T7 já satisfeito.

## Blockers
nenhum

## Notas para o próximo agente
- M1.T5 entregue: GOW1 removido inteiro do sistema. User confirmou "vai ser adicionado futuramente" — limpa agora, re-adiciona da forma correta depois.
- Enum `GameVersion::GOW1` removido (`src/core/types/GameVersion.h`). Comentário no header documenta intenção de re-adicionar com ProfileGOW1 + handlers/parsers completos quando for tempo.
- Removidos:
  - 16 `REGISTER_TYPE(GOW1, ...)` macros em 7 handler files
  - 5 blocos `RegisterByTag(GOW::GameVersion::GOW1, ...)` em StructuralHandlers.cpp
  - 3 classes `*HandlerGOW1` (ObjectHandlerGOW1, SoundHandlerGOW1, FlipbookHandlerGOW1)
  - Branch GOW1 em `TypeIdToSchemaString` (WadTypes.h)
  - `LoadFromArchiveGOW1` + struct `RawTocEntryGOW1` + auto-detect GOW1/GOW2 em `ProfileGOW2.cpp`
  - `ParseGOW1` em `ObjectParser` + constants `GOW1_HEADER_SIZE` / `MAGIC_GOW1`
  - Conditional arms `(version == GOW1)` em `MeshParser.cpp` (hardcoded GOW2 layout)
  - GOW1 path branch (`if (entry.size == 0x5C)`) em `InstanceParser.cpp`
  - GOW1 path branch (`if (!instData->objectName.empty())`) em `InstanceHandler.cpp`
  - GOW1 registration em InstanceHandler
- Restantes 26 menções `GOW1` em src/core/ são todas em comentários, docstrings ou strings de log — não-operacionais. AC do roadmap permite explicitamente ("exceto em comentários e docs").
- Campos órfãos preservados (não impactam build, futuro re-add aproveita):
  - `ObjectData::file0x20` / `file0x24` (preenchidos só no path GOW1 deletado)
  - `InstanceData::objectName` (mantido por compat com handler; sempre `""` em GOW2)
  - `static BuildTRSMatrix` em InstanceParser (helper TRS, agora sem caller)
- ctest 6/6 verde. Layer linter estável em 11 violações.

## Progresso M1
- T1 ✓ commit `6454eac`
- T2 ✓ commit `260c42a`
- T3 ✓ commit `8b0e7f2`
- T4 ✓ commit `044875d`
- T5 ✓ (commit pendente)
- T6 + Gate restantes

## Arquivos tocados nesta sessão (cumulativo)
- Hotfix font + actool: `fdeaed7`
- M1.T2 BoundingBox: `260c42a`
- M1.T3 MeshData decouple: `8b0e7f2`
- M1.T4 IAssetLoader dead-code: `044875d`
- M1.T5 GOW1 purge (12 files):
  - `src/core/types/GameVersion.h` (drop enum value + doc)
  - `src/core/WadTypes.h` (drop GOW1 branch)
  - `src/core/parsers/gow2/MeshParser.cpp` (hardcode GOW2 layout)
  - `src/core/parsers/gow2/InstanceParser.cpp` (drop 0x5C branch)
  - `src/core/parsers/gow2/ObjectParser.{h,cpp}` (drop ParseGOW1 + constants)
  - `src/core/profiles/gow2/ProfileGOW2.{h,cpp}` (drop LoadFromArchiveGOW1 + auto-detect)
  - `src/core/types/handlers/{Texture,Model,Mesh,Material,Gfx}Handler.cpp` (drop REGISTER_TYPE(GOW1))
  - `src/core/types/handlers/ObjectHandler.cpp` (drop ObjectHandlerGOW1 class + REGISTER)
  - `src/core/types/handlers/InstanceHandler.cpp` (drop GOW1 path + REGISTER)
  - `src/core/types/handlers/ContentHandlers.cpp` (drop 2 GOW1 classes + 8 REGISTER + section)
  - `src/core/types/handlers/StructuralHandlers.cpp` (drop 5 RegisterByTag blocks)
