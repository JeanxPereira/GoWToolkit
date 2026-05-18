#!/usr/bin/env python3
"""
make_test_fixtures.py — generate the minimal WAD fixtures consumed by the
unit-test suite.

The generator never produces full game assets. For GOW2 it truncates a real
WAD at a clean tag boundary so the slice remains a valid (just shorter)
sequential-tag archive. For GOWR it copies the source WAD verbatim because the
container relies on a `blockBitSet`-driven flush algorithm to resolve payload
offsets — any naive truncation would invalidate that resolution. The GOWR
source must therefore be a small WAD on its own (the project ships small UI /
character WADs that fit comfortably under the 1 MB hard cap).

Usage:
    python3 tools/make_test_fixtures.py \\
        --gow2 /path/to/SOURCE_GOW2.WAD \\
        --gowr /path/to/SOURCE_GOWR.wad \\
        --out  tests/fixtures

References:
    docs/GoW2/Formats/WAD.md   — GOW2 tag header layout
    docs/GoWRknk/Formats/Wad.md — GOWR WTOC + LZ4 framing
    src/core/profiles/gow2/ProfileGOW2.cpp — authoritative parser
    src/core/profiles/gowr/GOWRTypes.h     — WTOC / FileDesc struct sizes
"""
from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path

# ── GOW2 tag layout (32 bytes; little-endian) ─────────────────────────────────
# u16 tag | u16 flags | u32 size | char[24] name
GOW2_TAG_HEADER = struct.Struct("<HHI24s")
assert GOW2_TAG_HEADER.size == 32

GOW2_TAG_HEADER_START = 21   # WADTAG_HEADER_START
GOW2_TAG_GROUP_END    = 3    # WADTAG_GROUP_END

# Target sizes (bytes). The hard upper bound enforced by the M0.T2 acceptance
# criteria is 1 MB; we aim well below that.
GOW2_TARGET_BYTES = 256 * 1024
GOWR_HARD_LIMIT   = 1024 * 1024


def truncate_gow2(src: Path, dst: Path) -> int:
    """Walk the tag stream and truncate at the first 16-byte-aligned tag
    boundary at or past GOW2_TARGET_BYTES. Returns the size of the written
    fixture in bytes."""
    data = src.read_bytes()
    pos = 0
    last_safe_offset = 0   # offset where we could legally stop

    while pos + GOW2_TAG_HEADER.size <= len(data):
        tag, _flags, size, _name = GOW2_TAG_HEADER.unpack_from(data, pos)
        payload_end = pos + GOW2_TAG_HEADER.size + size
        # GOW2 tag stream is 16-byte aligned after each payload.
        aligned_end = (payload_end + 15) & ~15

        if aligned_end > len(data):
            # Tag would run past the source; bail out.
            break

        # A clean truncation point sits *after* this tag's payload + alignment.
        # Prefer cutting after a structural tag so the resulting fixture ends
        # on a parser-friendly boundary.
        if aligned_end >= GOW2_TARGET_BYTES and tag in (GOW2_TAG_GROUP_END, GOW2_TAG_HEADER_START):
            last_safe_offset = aligned_end
            break

        last_safe_offset = aligned_end
        pos = aligned_end

    if last_safe_offset == 0:
        raise RuntimeError(f"failed to find any clean tag boundary in {src}")

    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data[:last_safe_offset])
    return last_safe_offset


def copy_gowr(src: Path, dst: Path) -> int:
    """Copy a small GOWR WAD verbatim. GOWR offset resolution depends on a
    blockBitSet flush algorithm; any naive truncation invalidates it, so the
    fixture is shipped whole and the *source* is required to be small."""
    data = src.read_bytes()
    if len(data) > GOWR_HARD_LIMIT:
        raise RuntimeError(
            f"GOWR source {src} is {len(data)} bytes (> {GOWR_HARD_LIMIT}). "
            "Pick a smaller WAD: GOWR fixtures cannot be safely truncated."
        )
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data)
    return len(data)


def sha256_of(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def main(argv: list[str]) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--gow2", type=Path, required=True,
                   help="Path to a source GOW2 WAD (will be truncated).")
    p.add_argument("--gowr", type=Path, required=True,
                   help="Path to a small GOWR WAD (will be copied as-is; must fit under 1 MB).")
    p.add_argument("--out", type=Path, default=Path("tests/fixtures"),
                   help="Output root (default: tests/fixtures).")
    args = p.parse_args(argv)

    if not args.gow2.is_file():
        print(f"ERROR: --gow2 not found: {args.gow2}", file=sys.stderr)
        return 2
    if not args.gowr.is_file():
        print(f"ERROR: --gowr not found: {args.gowr}", file=sys.stderr)
        return 2

    gow2_dst = args.out / "gow2" / "wad_minimal.wad"
    gowr_dst = args.out / "gowr" / "wad_minimal.wad"

    gow2_bytes = truncate_gow2(args.gow2, gow2_dst)
    gowr_bytes = copy_gowr(args.gowr, gowr_dst)

    print(f"[GOW2] {gow2_dst}  {gow2_bytes:>7} bytes  sha256={sha256_of(gow2_dst)}")
    print(f"[GOWR] {gowr_dst}  {gowr_bytes:>7} bytes  sha256={sha256_of(gowr_dst)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
