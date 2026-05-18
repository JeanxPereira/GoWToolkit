#!/usr/bin/env python3
"""
check_layers.py — verifies that #include directives in src/ respect the
layered hierarchy defined in `tools/layers.yaml`.

How it works:
  1. Walk every `.h`, `.hpp`, `.cpp`, `.mm` under `src/` (skipping
     `third_party/`, `build/`, etc.).
  2. For each file, classify it into a layer by longest-prefix match
     against the `layers:` table.
  3. For every `#include "..."` or `#include <...>` line, try to
     resolve the included path to a file inside `src/`. Unresolved
     includes (system headers, fetched dependencies, third-party
     vendored headers) are ignored.
  4. Apply the `rules:` constraints (`LX must not include LY` and
     range form `LX..LY`). Print one annotation line per violation
     in the GitHub Actions `::warning file=...` format so the CI lint
     job surfaces them inline on PRs.

Exit code is 0 by default so the job can run as warning-only during
the M0/M1 transition; pass `--strict` to exit non-zero on any
violation (intended for the M2 promotion of this lint to a hard
gate).
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Iterable

import yaml

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+["<]([^">]+)[">]')
SOURCE_SUFFIXES = {".h", ".hpp", ".cpp", ".cc", ".cxx", ".mm", ".m"}

# Directories under repo root that we never inspect — they're either
# generated, vendored, or out-of-tree.
IGNORE_DIRS = {"build", "third_party", "_deps", ".git", "tests/fixtures"}


def is_ignored(path: Path, repo_root: Path) -> bool:
    try:
        rel = path.relative_to(repo_root).as_posix()
    except ValueError:
        return True
    return any(rel == d or rel.startswith(d + "/") for d in IGNORE_DIRS)


def iter_sources(repo_root: Path) -> Iterable[Path]:
    src = repo_root / "src"
    for p in src.rglob("*"):
        if not p.is_file():
            continue
        if is_ignored(p, repo_root):
            continue
        if p.suffix.lower() not in SOURCE_SUFFIXES:
            continue
        yield p


def classify(rel: str, layers: dict[str, list[str]]) -> str | None:
    """Return the layer name whose longest prefix matches `rel`.
    `rel` is the path relative to the repo root (POSIX separators).
    """
    best_layer: str | None = None
    best_len = -1
    for layer, prefixes in layers.items():
        for prefix in prefixes:
            if rel == prefix:
                # Exact match: most specific possible.
                if len(prefix) > best_len:
                    best_layer = layer
                    best_len = len(prefix)
                continue
            if rel.startswith(prefix + "/"):
                if len(prefix) > best_len:
                    best_layer = layer
                    best_len = len(prefix)
    return best_layer


def resolve_include(include_path: str, source_file: Path, repo_root: Path) -> Path | None:
    """Try to resolve `#include "x"` (or <x>) to a file under src/.
    Resolution order mirrors the project's compile options:
      1. Relative to the including file's directory.
      2. Relative to `src/` (target_include_directories = src).
    """
    candidates = [
        (source_file.parent / include_path).resolve(),
        (repo_root / "src" / include_path).resolve(),
    ]
    src_root = (repo_root / "src").resolve()
    for cand in candidates:
        try:
            cand.relative_to(src_root)
        except ValueError:
            continue
        if cand.is_file():
            return cand
    return None


def parse_includes(path: Path) -> list[tuple[int, str]]:
    out: list[tuple[int, str]] = []
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return out
    for idx, line in enumerate(text.splitlines(), start=1):
        m = INCLUDE_RE.match(line)
        if m:
            out.append((idx, m.group(1)))
    return out


def resolve_layer_token(token: str, layer_order: list[str]) -> str:
    """`token` is the short form used in `rules:` (e.g. "L0", "L2").
    Match it against the canonical layer keys (e.g. "L0_infra",
    "L2_domain") by case-insensitive prefix.
    """
    t = token.strip()
    matches = [L for L in layer_order if L == t or L.startswith(t + "_")]
    if len(matches) != 1:
        raise ValueError(f"rule references unknown layer token: {token!r} "
                         f"(matches={matches}, known={layer_order})")
    return matches[0]


def parse_rules(rules_raw: list[str], layer_order: list[str]) -> list[tuple[str, list[str]]]:
    """Each rule "LX must not include LY" or "LX must not include LY..LZ"
    is normalised to (source_layer, [forbidden_layers...])."""
    normalized: list[tuple[str, list[str]]] = []
    for rule in rules_raw:
        toks = rule.replace("must not include", "→").split("→")
        if len(toks) != 2:
            raise ValueError(f"unparseable rule: {rule!r}")
        src = resolve_layer_token(toks[0], layer_order)
        dst_raw = toks[1].strip()

        forbidden: list[str] = []
        if ".." in dst_raw:
            lo, hi = [resolve_layer_token(t, layer_order) for t in dst_raw.split("..")]
            i, j = layer_order.index(lo), layer_order.index(hi)
            if i > j:
                i, j = j, i
            forbidden = layer_order[i : j + 1]
        else:
            forbidden = [resolve_layer_token(dst_raw, layer_order)]
        normalized.append((src, forbidden))
    return normalized


def main(argv: list[str]) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--config", type=Path, default=Path(__file__).with_name("layers.yaml"))
    p.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument("--strict", action="store_true",
                   help="Exit non-zero when at least one violation is found.")
    p.add_argument("--format", choices=("text", "github"), default="text")
    args = p.parse_args(argv)

    repo_root = args.repo_root.resolve()

    with args.config.open() as f:
        cfg = yaml.safe_load(f)

    layers: dict[str, list[str]] = cfg.get("layers") or {}
    rules_raw: list[str] = cfg.get("rules") or []
    if not layers:
        print("error: layers.yaml has no `layers:` block", file=sys.stderr)
        return 2

    layer_order = list(layers.keys())
    rules = parse_rules(rules_raw, layer_order)

    src_root = (repo_root / "src").resolve()
    if not src_root.is_dir():
        print(f"error: src/ directory not found: {src_root}", file=sys.stderr)
        return 2

    violations = 0
    for source in iter_sources(repo_root):
        rel_source = source.resolve().relative_to(repo_root).as_posix()
        source_layer = classify(rel_source, layers)
        if not source_layer:
            continue
        for line_no, inc in parse_includes(source):
            target = resolve_include(inc, source, repo_root)
            if target is None:
                continue
            rel_target = target.relative_to(repo_root).as_posix()
            target_layer = classify(rel_target, layers)
            if not target_layer:
                continue
            for rule_src, forbidden in rules:
                if source_layer == rule_src and target_layer in forbidden:
                    violations += 1
                    msg = (f"layer violation: {source_layer} → {target_layer} "
                           f"({rel_source}:{line_no} includes {rel_target})")
                    if args.format == "github":
                        print(f"::warning file={rel_source},line={line_no}::{msg}")
                    else:
                        print(msg)

    print(f"check_layers: {violations} violation(s) found", file=sys.stderr)
    if args.strict and violations > 0:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
