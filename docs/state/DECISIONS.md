# Decisões Pontuais

> Para decisões que **não merecem ADR formal** mas precisam ficar registradas.
> ADRs vivem em `docs/ADR/NNNN-slug.md` (formato Nygard).
> Aqui ficam: trade-offs pequenos, escolhas pontuais de biblioteca, exceções a regras.

---

## D0001 — 2026-05-17 — Histórico do bootstrap

Decisão: consolidar WIP pré-existente em `main` em três commits lógicos (chore → feat → docs), em vez de stash ou mistura com branch refactor.

Razão: histórico legível separa "groundskeeping" de "refator estrutural"; permite que `git blame` aponte para mudanças semanticamente coerentes.

Trade-off: o WIP de parser GOWR não tem testes (pré-existente), mas isso é endereçado por M0.T3 (golden test snapshot) que vai cobrir esses parsers logo na sequência.

---

## D0002 — 2026-05-17 — Co-author tag dos commits

Decisão: usar `Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>` conforme convenção atual do harness.

Razão: identificável em `git log`; permite filtrar contribuições assistidas vs manuais.

---

## D0003 — 2026-05-18 — Fixtures de golden test (origem das WADs)

Decisão: fixtures vêm de **truncamento de WADs comerciais reais**, mantidas abaixo do teto de 1 MB do acceptance criteria de M0.T2.

Estratégias adotadas (por jogo, refletindo a diferença de formato):
- **GOW2** (`tests/fixtures/gow2/wad_minimal.wad`, 265 KB): truncamento tag-aware via `tools/make_test_fixtures.py`. O script percorre o stream sequencial documentado em `docs/GoW2/Formats/WAD.md` e corta em boundary alinhada a 16 bytes ao primeiro tag estrutural (`GROUP_END` ou `HEADER_START`) após 256 KB. Resultado preserva header + primeiros server instances / groupings.
- **GOWR** (`tests/fixtures/gowr/wad_minimal.wad`, 538 KB): cópia íntegra de uma WAD per-character pequena (`r_athena00.wad`). Truncamento naive quebraria offset resolution (algoritmo `blockBitSet`/flush em `ProfileGOWR::ParseWad`), então o source precisa ser pequeno por construção.

Trade-off escolhido: **fidelidade máxima ao formato real** (mantém parser exercitado em bytes que o jogo de fato emite). Risco de redistribuição mitigado pelo tamanho mínimo (ambos < 1 MB; só headers + handful de assets pequenos) e disclaimer explícito em `tests/fixtures/README.md`.

Origens dos sources (não-versionadas):
- GOW2: `R_BOAR00.WAD` extraído manualmente da ISO PS2 USA.
- GOWR: `r_athena00.wad` da árvore `exec/wad/pc_le/` do GoWR PC.

SHA-256 dos fixtures registrados em `tests/fixtures/README.md`. Regeneração: `python3 tools/make_test_fixtures.py --gow2 <path> --gowr <path>`.

---

## D0004 — 2026-05-18 — Extração de `gowtoolkit_parser_min` static lib

Decisão: criar `gowtoolkit_parser_min` como static library separada em `src/core/` containing apenas profiles + WAD plumbing (Logger, WadAssetName, schema, types, vfs, profiles/*). UI/handlers/loaders/parsers content-side ficam fora — só o main exe os compila.

Motivação: M0.T3 precisa rodar `ProfileGOW2::ParseWad` e `ProfileGOWR::ParseWad` do test binary sem arrastar a UI (ImGui, GLFW, viewers). A camada atual `src/core/{loaders,types/handlers}/*` viola layering ao includar `ui/viewers/*` — bug separado endereçado em M0.T8 (layer linter). Pra desbloquear M0.T3 sem fazer aquele refator inteiro, extraio só o que parser precisa.

Trade-off escolhido: lib pequena, foco cirúrgico, sem mudar produção da UI. Custo: lista hardcoded de sources no `CMakeLists.txt` (12 arquivos). Adicionar parser novo em `src/core/profiles/` requer atualizar `PARSER_MIN_SOURCES`.

Símbolo `GetTexIndex()` (declarado em GOWRLoaders.h, definido em GOWRLoaders.cpp que **não** entra no parser-min) é stubado em `tests/test_stubs.cpp`. Production exe segue usando a implementação real.

Refs:
- `CMakeLists.txt` (bloco `gowtoolkit_parser_min`)
- `tests/test_stubs.cpp`
- M0.T8 (layer linter) eventualmente formalizará a regra "core não pode includar ui".

---

## D0005 — 2026-05-18 — Hash do snapshot é xxhash64, não SHA-1

Decisão: `SnapshotEntries` usa `XXH64` (do `xxhash.h` já vendored via `lz4_lib`) em vez de SHA-1 como o roadmap M0.T3 originalmente sugeriu.

Motivação: xxhash já está linkado (lz4 carrega xxhash internamente), zero deps extra; é determinístico cross-platform; muito mais rápido. Não precisamos de propriedades criptográficas — só estabilidade pra regression detection.

Trade-off: divergência do roadmap em 1 detalhe. Sem custo prático — qualquer hash determinístico cumpriria a função.

---

## D0006 — 2026-05-18 — Metrics API usa `uint32_t`, não `TypeId`

Decisão: `GOW::Metrics::RecordParseTime` aceita `uint32_t typeId` em vez de `GOW::TypeId` como o roadmap M0.T4 step 1 sugere.

Motivação: AC #4 de M0.T4 restringe os includes de `Metrics.h` a `<chrono>, <string>, <map>, <mutex>` (+ `<cstdint>` adicionado para os fixed-width int types). Aceitar `TypeId` por valor + usar `std::map<TypeId, ...>` no `Snapshot` exige `TypeId.h` no header. Caller usa `static_cast<uint32_t>(typeId)`.

Trade-off: 1 cast por call site. Em troca: `Metrics.h` é um leaf module sem deps do projeto, pode ser dropado em qualquer TU sem arrastar core.

---

## D0007 — 2026-05-18 — Custo de `RecordParseTime` disabled

Decisão: AC literal "< 5 ns por call quando disabled" foi medido em Debug build (`-O0`) ≈ 8 ns/call. Em Release com inlining espera-se < 1 ns (single atomic.load + branch). Test mantém ceiling de **500 ns** pra robustez em CI compartilhada.

Motivação: 5 ns é instable em Debug e em hardware lento (CI). 500 ns trava regressões reais (qualquer adição acidental de lock/map lookup ao hot path saltaria para µs). Hot path validado: 1 atomic load + branch + return.

Refs:
- `tests/metrics_test.cpp` — `TEST_CASE("[Metrics] RecordParseTime is cheap when disabled")`
- Benchmark output: `[Metrics] disabled RecordParseTime cost ~8 ns/call` (Debug build)

---

## D0008 — 2026-05-18 — Logger usa `std::format`, não `fmtlib`

Decisão: `GOW::Log::Log` aceita `std::format_string<Args...>` em vez de `fmt::format_string<Args...>` como o roadmap M0.T5 step 1 sugere. Sem dependência externa nova.

Motivação: Apple Clang 21.0.0 (toolchain atual) regride no path `consteval` de `fmt::basic_format_string::basic_format_string<FMT_COMPILE_STRING, 0>` quando fmt está em modo header-only ou compilado. Erros são internos a `format-inl.h` (não código do projeto). Testes:
- fmt 10.2.1 + static lib: ❌ consteval fail em `os.cc`, `format-inl.h`
- fmt 11.0.2 + static lib: ❌ mesmo problema
- fmt 11.0.2 + header-only + `FMT_USE_CONSTEVAL=0`: ❌ FMT_STRING internal ainda dispara consteval
- `std::format` libc++ embarcado no toolchain: ✓ funciona

`std::format` cobre 100% do uso planejado (format string positional `"{}"`, levels, sinks). API alignment: drop-in syntax. Custo zero em deps.

Trade-off: `std::format` requer C++20 libc++ (`std::format` GA em 16+). Já é requisito do projeto. Se mais tarde precisarmos de features que std::format não tem (named args, custom formatters mais ricos), reavaliamos fmt.

Refs:
- `src/core/Logger.h` — `template<...> void Log(Level, sv, std::format_string<Args...>, Args&&...)`
- `tests/logger_test.cpp` — confirma `"[INFO][test] hello 42"` literal AC
- `clang++ --version` → `Apple clang version 21.0.0 (clang-2100.1.1.101)`

---

## D0009 — 2026-05-18 — Manter `Logger` facade legacy + macros `LOG_*`

Decisão: novo `GOW::Log` namespace coexiste com `GOW::Logger` antigo e os macros `LOG_DEBUG/INFO/WARN/ERR`. Migração de call sites é **gradual**, não big-bang.

Motivação: existem ~250 call sites de `LOG_*` espalhados. Rewriting all em M0.T5 violaria escopo (AC: "Substituir `fprintf(stderr, ...)` em EventManager.h **apenas**"). Solução:

- `Logger::Log()` (printf-style) → formata buffer → chama `Log::LogString()` com category=""
- Ambos funnel pelos mesmos sinks
- Sink in-memory sempre presente, alimenta `Logger::GetEntries()` para `StatusBar` UI
- `LOG_*` macros mantidos como aliases printf-style sem categoria
- `GOW_LOG_*` macros são o caminho preferido pra código novo (com categoria)

Trade-off: API surface dupla. Aceito porque migration paths são separadas. Plano: tasks futuras podem migrar arquivo-a-arquivo para `GOW_LOG_*` sem coordenação.

Refs:
- `src/core/Logger.h` — legacy `Logger` class + new `Log` namespace lado a lado
- `tests/logger_test.cpp` — `TEST_CASE("[Logger] Legacy LOG_INFO funnels through the new pipeline")`
