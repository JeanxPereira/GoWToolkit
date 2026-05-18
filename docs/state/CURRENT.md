# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão de continuação)
**Sessão #**: 2

## Milestone ativa
M0 — Safety Net

## Task em progresso
nenhuma — M0.T5 fechada, próxima é M0.T6

## Próxima task no pipeline
M0.T6 — clang-format + EditorConfig + CI workflow
- **Prereqs**: nenhum (paraleliza com M0.T1–T5)
- Arquivos: `.clang-format`, `.editorconfig`, `.github/workflows/ci.yml`, `.github/pull_request_template.md`

## Blockers
nenhum

## Notas para o próximo agente
- Toolchain: Apple Clang 21.0.0 (Xcode 26). std::format funciona; fmtlib quebra em FMT_STRING consteval — ver D0008.
- Logger pipeline: `GOW_LOG_*` (categorizado) e `LOG_*` (legacy) AMBOS funnel pelos mesmos sinks. UI lê via `GOW::Logger::Get().GetEntries()`.
- `Log::SetMinLevel(Level)` filtra runtime. Default: Info. Trace e Debug filtrados por default.
- `Log::AddSink(fn)` returna `SinkToken`; `Log::RemoveSink(token)` desinstala. Default: nenhum sink (apenas in-memory ring sempre ligado).
- `Log::InstallStderrSink()` e `Log::InstallRotatingFileSink(path, max, rotations)` são opt-in installers. Não chamados em main.cpp ainda — main exe agora SILENCIA stderr para logs (UI mostra tudo via StatusBar). Próximo agente pode adicionar `Log::InstallStderrSink()` em `App::App()` se quiser eco.
- M0.T6: precisa adicionar `.clang-format`. Style: ver `src/core/Logger.h` (existing patterns: 4-space indent, brace style já presente). Definir nas decisions se decisão sobre style ambíguo.
- M0.T6 CI workflow precisa rodar ctest no Linux runner. `pkg-config libavformat-dev libavcodec-dev libswscale-dev libswresample-dev libavutil-dev` necessário (CMakeLists.txt:154).
- Roadmap completo em `docs/ROADMAP_IMPLEMENTATION.md` §M0.T6.

## Arquivos tocados nesta sessão (acumulado)
### M0.T1: doctest scaffold
### M0.T2: WAD fixtures + truncator
### M0.T3: golden runner + parser-min lib + nlohmann/json
### M0.T4: Metrics opt-in
### M0.T5
- `src/core/Logger.{h,cpp}` (rewrite — Log namespace, sinks, std::format-based)
- `src/core/EventManager.h` (3 fprintf → GOW_LOG_*)
- `tests/logger_test.cpp` (NEW — 6 TEST_CASEs)
- `CMakeLists.txt` (Logger ctest entry)
