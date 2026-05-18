# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão de continuação)
**Sessão #**: 2

## Milestone ativa
M0 — Safety Net

## Task em progresso
nenhuma — M0.T1 fechada, aguardando início de M0.T2

## Próxima task no pipeline
M0.T2 — Golden Test Fixtures (mínimos)
- **Bloqueada por**: D0003 (decisão humana sobre origem das WADs sintéticas vs truncadas)
- **Prereq satisfeito**: M0.T1 ✓

## Blockers
- D0003 (M0.T2) — aguardando confirmação humana antes de gerar fixtures binárias.

## Notas para o próximo agente
- doctest v2.4.11 fetched via `FetchContent`; opt-out via `-DGOWTOOLKIT_BUILD_TESTS=OFF`.
- Patch `CMAKE_POLICY_VERSION_MINIMUM=3.5` aplicado em volta do `FetchContent_MakeAvailable(doctest)` porque o CMakeLists do upstream tem `cmake_minimum_required(<3.5)`. Remover quando doctest publicar release com floor atualizado.
- `add_test(NAME unit COMMAND $<TARGET_FILE:gowtoolkit_tests>)` — usar generator expression sempre (sem isso, ctest não acha o exe quando o build ainda não rodou todos os targets).
- `tests/CMakeLists.txt` usa `file(GLOB CONFIGURE_DEPENDS tests/*.cpp)` — adicionar `.cpp` no diretório já entra no build sem editar CMake.
- Release build pré-existente quebra em macOS dev sem Xcode full (`xcrun actool` ausente). Não é regressão do M0.T1 — Debug build segue verde.
- M0.T2 precisa ANTES da implementação: gerar / capturar `wad_minimal.wad` para GoW2 e GoWR. Parar e perguntar ao humano sobre origem (ver §M0.T2 ROADMAP + DECISIONS D0003).
- Roadmap completo em `docs/ROADMAP_IMPLEMENTATION.md`. Ler §0 e §1-§2 antes de qualquer task.
- Anti-padrão a evitar: começar M0.T3 antes de M0.T2 fechar (golden runner precisa de fixtures).

## Arquivos tocados nesta sessão
- `docs/state/CURRENT.md` (atualizado)
- `docs/state/COMPLETED.md` (apêndice M0.T1)
- `CMakeLists.txt` (FetchContent doctest, `enable_testing` via CTest, `add_subdirectory(tests)`)
- `tests/CMakeLists.txt` (NEW)
- `tests/main.cpp` (NEW)
- `tests/sanity_test.cpp` (NEW)
- `docs/GoW1/Formats/*` (commit prep — reorg)
- `docs/GoW2/Formats/*` (commit prep — reorg vindo de `docs/formats/GOW2/`)
- `docs/GoWRknk/{README,FORMAT_TEMPLATE}.md` (commit prep)
- `docs/GoWRknk/Formats/{Material,Mesh,Model,Shader,Skeleton,Texture,Wad}.md` (commit prep)
- `docs/FORMAT_TEMPLATE.md` (rename de `docs/formats/FORMAT_TEMPLATE.md`)
