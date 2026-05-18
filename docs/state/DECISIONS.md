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
