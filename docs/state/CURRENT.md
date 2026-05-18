# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão de continuação)
**Sessão #**: 2

## Milestone ativa
M0 — Safety Net

## Task em progresso
nenhuma — M0.T6 fechada, próxima é M0.T7

## Próxima task no pipeline
M0.T7 — ASSERT_MAIN_THREAD macro
- **Prereqs**: nenhum
- Arquivos: `src/core/Threading.{h,cpp}` (NEW); chamada `Threading::MarkMainThread()` no `main.cpp`
- Não adicionar `ASSERT_MAIN_THREAD()` em call-sites ainda (M1+)

## Blockers
nenhum

## Notas para o próximo agente
- `.clang-format` está commitado mas codebase **não** foi auto-formatado em massa. CI lint job roda dry-run em warning-only. Format incremental por feature/PR.
- `third_party/.clang-format` define `DisableFormat: true` — evita reformat de vendored sources.
- CI agora roda `ctest` em Linux/macOS/Windows. Windows usa multi-config (`-C Release`). Falha de golden test bloqueia CI.
- Lint job é `continue-on-error: true` — NÃO bloqueia merge. Flippar pra strict quando codebase estiver clean.
- PR template em `.github/pull_request_template.md` aparece em `gh pr create` automaticamente.
- M0.T7 (Threading.h) deve ser ADICIONADO ao `PARSER_MIN_SOURCES` no CMakeLists se entrar em src/core/ e for usado pelo parser (improvável — é só helper de assertion).
- Roadmap completo em `docs/ROADMAP_IMPLEMENTATION.md` §M0.T7.

## Arquivos tocados nesta sessão (acumulado)
### M0.T1–T5: ver entries anteriores

### M0.T6
- `.clang-format` (NEW — LLVM-based, 4-space, 100 col)
- `third_party/.clang-format` (NEW — DisableFormat)
- `.editorconfig` (NEW — LF, trim trailing, 4-space)
- `.github/workflows/ci.yml` (add ctest step per OS + lint job warning-only)
- `.github/pull_request_template.md` (NEW)
