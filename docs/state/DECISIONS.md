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

## D0003 — pendente — Fixtures de golden test (origem das WADs)

Pergunta aberta (M0.T2): as fixtures binárias em `tests/fixtures/{gow2,gowr}/` podem vir de truncamento de WADs comerciais reais, ou precisam ser 100% sintéticas (montadas no formato sem dados originais)?

Trade-offs:
- **Truncadas**: fidelidade máxima ao formato real; baixo esforço; risco de redistribuição de bytes proprietários (mesmo que < 1 MB).
- **Sintéticas**: zero risco legal; alto esforço (montar formato manualmente); risco de divergência sutil em relação ao formato real do jogo.

Próximo agente em M0.T2: **parar e perguntar ao humano antes de criar fixture**. Documentar a decisão aqui antes de prosseguir.
