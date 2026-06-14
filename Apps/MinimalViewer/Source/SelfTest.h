#pragma once
namespace MinimalViewer {
// Runs the headless self-test: registers a toy type, reads `path` via the
// public VFS, formats a hex dump, and validates the round-trip. Returns 0 on
// success, non-zero on failure. Used by --selftest and by ctest.
int RunSelfTest(const char* path);
}
