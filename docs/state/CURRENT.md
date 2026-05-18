# Estado Atual

**Última atualização**: 2026-05-17 por Claude (sessão de bootstrap)
**Sessão #**: 1

## Milestone ativa
M0 — Safety Net

## Task em progresso
M0.T1 — Setup doctest
- **Status**: in_progress
- **Branch**: `refactor/m0-safety-net`
- **Iniciada em**: 2026-05-17
- **% estimado**: 0

### Subtasks (M0.T1)
- [ ] M0.T1.S1 — Add doctest v2.4.11 via FetchContent in root `CMakeLists.txt`
- [ ] M0.T1.S2 — Create `tests/CMakeLists.txt` defining `gowtoolkit_tests` executable
- [ ] M0.T1.S3 — Add `enable_testing()` + `add_test(NAME unit COMMAND gowtoolkit_tests)` in root
- [ ] M0.T1.S4 — Create `tests/main.cpp` with `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`
- [ ] M0.T1.S5 — Create `tests/sanity_test.cpp` with smoke `TEST_CASE`
- [ ] M0.T1.S6 — Verify `cmake -G Ninja -B build && ninja -C build && ctest --test-dir build`
- [ ] M0.T1.S7 — Commit + update `CURRENT.md` + append `COMPLETED.md`

## Próxima task no pipeline
M0.T2 — Golden Test Fixtures (mínimos)

## Blockers
nenhum

## Notas para o próximo agente
- Bootstrap concluído: 4 commits em `main` (gitignore + parser WIP + arch docs + extra format docs).
- Branch `refactor/m0-safety-net` criada a partir de `main` (commit `c33cc1c`).
- `docs/state/` recém-criado nesta sessão. Sempre atualizar antes de encerrar.
- M0.T2 (golden fixtures) depende de decisão humana sobre origem das WADs sintéticas vs truncadas — ver §M0.T2 do ROADMAP_IMPLEMENTATION.md (decisão pendente flag).
- Roadmap completo em `docs/ROADMAP_IMPLEMENTATION.md`. Ler §0 e §1-§2 antes de qualquer task.
- Anti-padrão a evitar: começar M0.T2 antes de M0.T1 fechar (doctest precisa estar funcional).

## Arquivos tocados nesta sessão
- `.gitignore` (added scratch patterns)
- `docs/GOWR-Modding-Guide.md` (DELETED)
- `docs/GOWR_PLANNING.MD` (DELETED)
- `docs/ARCHITECTURE_REVIEW_2026-05.md` (NEW)
- `docs/ROADMAP_IMPLEMENTATION.md` (NEW)
- `docs/state/CURRENT.md` (NEW — this file)
- `docs/state/COMPLETED.md` (NEW)
- `docs/state/DECISIONS.md` (NEW)
