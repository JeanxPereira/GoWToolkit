# God of War Ragnarök (GoWR) Analysis & Format Documentation

## Core Disclaimer
> [!IMPORTANT]
> The documentation and structural analysis contained within this directory are **explicitly targeted at the PC release of God of War Ragnarök (`GoWR.exe`)**. 
> While we utilize reference tools (`GOWTool` and `GoWRknk`) that previously handled the PS4 and PS5 versions, the primary memory alignments, pointers, offsets, and algorithms described here are verified via `ghidra-mcp` executing against the PC binary as the definitive source of truth.

## Tooling Pipeline & Analysis Strategy
Our reverse engineering pipeline for Ragnarök utilizes a tri-fold approach:

1. **`GOWTool` (C++)**: Extracts the hierarchical logic of WAD arrays, textures (`Texpack.cpp`), and WTOC structures.
2. **`GoWRknk` (C#)**: Supplies the mathematical decoding algorithms (like `morton` curves) and fixed-point Quaternion normalizations used in skeletal binding (`PROTO`).
3. **`Ghidra` (GoWR.exe PC)**: Validates the physical byte offsets and guarantees the struct schemas represent exactly what is loaded in the PC engine memory pool.

## Format Directory
All specific format documentation is maintained under the `/Formats/` directory. These specifications enforce strict table alignments and Mermaid mapping diagrams to ensure 1:1 C++ struct porting in the `GOWToolkit` engine.
