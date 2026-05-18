#!/usr/bin/env bash
# Regenerate the golden snapshot JSONs that the unit-test suite compares
# parser output against. Run this whenever a parser change *intentionally*
# alters the on-disk schema captured by `SnapshotEntries`.
#
# Usage:
#     tools/regenerate_goldens.sh                # auto-detect build/
#     tools/regenerate_goldens.sh path/to/build  # use an explicit build dir
#
# The script:
#   1. Builds the `gowtoolkit_tests` target (no-op if already up to date).
#   2. Runs the golden-test case binary with `GOWTOOLKIT_GOLDEN_UPDATE=1`,
#      which causes each test to overwrite its `*.expected.json` instead
#      of comparing.
#
# After it finishes, `git diff tests/fixtures/**/*.expected.json` shows
# exactly what your parser change moved.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-${REPO_ROOT}/build}"

if [[ ! -d "${BUILD_DIR}" ]]; then
    echo "error: build directory not found: ${BUILD_DIR}" >&2
    echo "       run cmake -G Ninja -B build first, or pass a path explicitly." >&2
    exit 2
fi

echo "[regenerate-goldens] building gowtoolkit_tests in ${BUILD_DIR}"
cmake --build "${BUILD_DIR}" --target gowtoolkit_tests

TEST_BIN="${BUILD_DIR}/tests/gowtoolkit_tests"
if [[ ! -x "${TEST_BIN}" ]]; then
    echo "error: test binary not found or not executable: ${TEST_BIN}" >&2
    exit 3
fi

echo "[regenerate-goldens] writing fresh JSON snapshots"
GOWTOOLKIT_GOLDEN_UPDATE=1 "${TEST_BIN}" --test-case='[Golden]*'

echo "[regenerate-goldens] done. Review changes with:"
echo "    git diff tests/fixtures/**/*.expected.json"
