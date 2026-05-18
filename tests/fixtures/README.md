# Test Fixtures

This directory holds the minimal binary WAD inputs consumed by the golden
test runner (see `tests/golden_*.cpp`). Fixtures are **versioned** so the
test suite is hermetic; they must stay small and not redistribute large
commercial blobs.

## Files

| Path | Bytes | SHA-256 | Origin |
|------|-------|---------|--------|
| `gow2/wad_minimal.wad` | 265,936 | `89098731400c1a5a126466bb863c99921b06a8aaecbdca3ddcca31b7dd339ee6` | Tag-aware truncation of `R_BOAR00.WAD` (GOW2 PS2 USA). |
| `gowr/wad_minimal.wad` | 538,410 | `c01dfe5ac128e30bc7031342c0c5726f954de04042ec92ed47b322c024331e3e` | Verbatim copy of `r_athena00.wad` (God of War Ragnarök PC). |

Both fixtures stay below the M0.T2 acceptance ceiling of 1 MB. The GOW2
fixture stays below the project target of 500 KB; the GOWR fixture is
larger because GOWR offset resolution depends on a `blockBitSet` flush
algorithm — truncating the payload region would invalidate that
resolution, so the *source* WAD must itself be small (`r_athena00.wad`
is a small per-character WAD, already 538 KB on disk).

## Provenance

The fixtures are derived from commercial WAD files that **must not** be
committed to this repository. Only the truncated / small slices below
the 1 MB cap are versioned, and only the headers + a handful of small
asset entries are retained.

### GOW2 — `gow2/wad_minimal.wad`

Source: `R_BOAR00.WAD` (extracted by hand from the GOW2 PS2 USA ISO).
The script walks the sequential tag stream described in
[`docs/GoW2/Formats/WAD.md`](../../docs/GoW2/Formats/WAD.md) and stops
at the first 16-byte-aligned boundary at or past 256 KB that lands on a
`GROUP_END` or `HEADER_START` tag. The resulting slice retains the
WAD's structural header plus the first few server instances /
groupings, which exercise the `WADTAG_HEADER_START`, `WADTAG_GROUP_*`,
and `WADTAG_SERVER_INST` paths in `ProfileGOW2::ParseWad`.

### GOWR — `gowr/wad_minimal.wad`

Source: `r_athena00.wad`. The file is shipped as-is. The GOWR WAD layout
(`docs/GoWRknk/Formats/Wad.md`, `src/core/profiles/gowr/GOWRTypes.h`) is
LZ4-framed and payload offsets are reconstructed at parse time by the
`blockBitSet` flush algorithm in `ProfileGOWR::ParseWad`. Modifying the
`FileDesc` array invalidates those offsets, so naive truncation is not
viable.

## Regenerating

The fixtures are produced deterministically by
[`tools/make_test_fixtures.py`](../../tools/make_test_fixtures.py). If
the source WADs change (e.g., a different region dump) or the
truncation strategy changes, re-run:

```sh
python3 tools/make_test_fixtures.py \
    --gow2 /path/to/your/SOURCE_GOW2.WAD \
    --gowr /path/to/your/SOURCE_GOWR.wad \
    --out  tests/fixtures
```

Then update the SHA-256 hashes in this README and bump the expected
JSON snapshots in `tests/fixtures/*/wad_minimal.expected.json` (added
by M0.T3 — see `docs/ROADMAP_IMPLEMENTATION.md`).

## Licensing

These fixtures contain proprietary bytes derived from Sony Santa
Monica's God of War II and God of War Ragnarök WAD archives. They are
included here strictly as **test inputs** under fair use for software
interoperability and reverse-engineering research. Do not redistribute
the source WADs from which they were derived.
