# TWK Format Specification (GOW2)

## Overview
The TWK (Tweak) format implements an internal Virtual File System (VFS). It's used by developers to configure generic gameplay variables, AI properties, combat states, and tunable variables ("tweaks") using a structured hierarchical dictionary.

## Architecture & Hierarchy
It does not have a fixed struct representation. Instead, it uses a byte-stream parser to construct a directory tree in memory.

## Header Structure
The format uses Tag ID `0x71` for standard tweaks and `0x72` for Combat Files.
If the first `u32` is `0xFEDCBA98` (`-0x1234568`), it indicates an extended magic header, followed by a unique UID, with the actual stream starting at `0x8`.

## Byte Stream Commands
The parser loops over bytes, extracting commands via a bitmask `cmdFlags = byte & 0xF0`:

| Command | Action | Description |
|---------|--------|-------------|
| `0x00`  | **End** | Closes the VFS parsing. Validates the directory stack is empty. |
| `0x80`  | **Root** | Reads a null-terminated string defining the root path (e.g., `/tweaks`). |
| `0x10`  | **Enter Dir** | Reads a null-terminated string and enters a sub-directory, pushing the current context onto a stack. |
| `0x40`  | **Exit Dir** | Pops the stack (equivalent to `cd ..`). |
| `0x20`  | **Set Field** | Reads a `u32` Hash ID representing the variable name. Then determines the data size via `byte & 0x0F` (acting as an index to `[4, 0x20, 0x40, 0x100, 0x200, 0x400, 0x800, 0x1000]`), and reads that many bytes into the value. |

## Idiosyncrasies
- **Combat Files**: Tag `0x72` behaves slightly differently, bypassing the stream parser and instead loading raw data directly into the VFS path named inside the file.
- **Hashing**: Names inside `0x20` commands are hashed. The engine relies on collision-less string hashes to match the tweaks.
