<!--
Thanks for sending a PR! Keep the description tight; the linked milestone
task carries the deeper context.
-->

## Summary

<!-- 1–3 bullets: what changed and why. Skip the diff narration. -->
-

## Linked task

<!--
Reference the milestone task this PR resolves. Example:
- Roadmap: `docs/ROADMAP_IMPLEMENTATION.md` §M0.T6
- COMPLETED entry to append after merge: yes / no
If this PR does NOT close a roadmap task, explain why here.
-->
Milestone / Task: <!-- e.g. M0.T6 — clang-format + EditorConfig + CI workflow -->

## Acceptance criteria

<!-- Tick the ACs from the roadmap section that this PR satisfies. -->
- [ ]
- [ ]

## Test plan

<!--
Bulleted checklist of what was actually verified locally. Be concrete
(commands, fixtures, observed output) — "ran tests" is not a plan.
-->
- [ ] `ctest --test-dir build` green locally
- [ ]

## Risk / Rollback

<!--
Anything reviewers should look at carefully (cross-cutting changes,
new dependencies, ABI shifts). One-line rollback strategy.
-->
- Risk:
- Rollback:

## State files

<!-- For roadmap tasks: -->
- [ ] `docs/state/CURRENT.md` updated for the next agent
- [ ] `docs/state/COMPLETED.md` appended (after merge)
- [ ] `docs/state/DECISIONS.md` entry added (if a non-ADR decision was made)
