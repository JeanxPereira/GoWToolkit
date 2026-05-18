# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão de continuação)
**Sessão #**: 2

## Milestone ativa
M0 — Safety Net

## Task em progresso
nenhuma — M0.T2 fechada, próxima é M0.T3

## Próxima task no pipeline
M0.T3 — SnapshotEntries + Golden Test runner
- **Prereqs satisfeitos**: M0.T1 ✓, M0.T2 ✓
- Arquivos novos esperados: `tests/golden_helpers.{h,cpp}`, `tests/golden_gow2.cpp`, `tests/golden_gowr.cpp`, `tests/fixtures/{gow2,gowr}/wad_minimal.expected.json`, `tools/regenerate_goldens.sh`.

## Blockers
nenhum

## Notas para o próximo agente
- Fixtures geradas via `tools/make_test_fixtures.py` (idempotente). Sources não estão no repo; ver `tests/fixtures/README.md` pra regenerar.
- GOW2 fixture parseável end-to-end (validado walking dos primeiros 8 tags); GOWR fixture é cópia íntegra de `r_athena00.wad` (538 KB) porque `blockBitSet` flush impede truncamento naive.
- **Atenção M0.T3**: parser GOWR depende de lz4frame (já compilado). Parser GOW2 não precisa de VFS pra estes fixtures — basta `IFile` com bytes do arquivo. Usar `MemoryFile` ou um wrapper de `std::ifstream`.
- SnapshotEntries deve ser **stable**: ordenar por offset; campos `name/typeId/size/offset/hash/childCount`; NÃO incluir `displayName`. JSON com indent fixo pra diff legível.
- `tools/regenerate_goldens.sh` deve aceitar nenhum argumento (usa paths default) e regenerar `*.expected.json` rodando o test binary com flag tipo `--update-goldens`.
- AC crítico (M0.T3): mutação de 1 byte no parser → ctest falha com diff legível mostrando o campo divergente.
- Roadmap completo em `docs/ROADMAP_IMPLEMENTATION.md` §M0.T3 (linha ~314).

## Arquivos tocados nesta sessão
### Pre-M0.T1 housekeeping
- `docs/state/{CURRENT,COMPLETED,DECISIONS}.md` (state scaffold commit)
- `docs/{FORMAT_TEMPLATE,GoW1/Formats/*,GoW2/Formats/*,GoWRknk/*}.md` (reorg commit)

### M0.T1
- `CMakeLists.txt` (doctest FetchContent, CTest, add_subdirectory(tests))
- `tests/CMakeLists.txt`, `tests/main.cpp`, `tests/sanity_test.cpp`

### M0.T2
- `tools/make_test_fixtures.py` (NEW)
- `tests/fixtures/gow2/wad_minimal.wad` (NEW — 265 KB, truncated)
- `tests/fixtures/gowr/wad_minimal.wad` (NEW — 538 KB, copy)
- `tests/fixtures/README.md` (NEW)
- `.gitattributes` (NEW — marks *.wad and other game assets binary)
- `docs/state/DECISIONS.md` (D0003 resolvido)
- `docs/state/CURRENT.md` (este arquivo)
- `docs/state/COMPLETED.md` (apêndice M0.T2)
