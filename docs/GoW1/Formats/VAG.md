# VAG Format Specification (GOW1)

## Overview
The VAG format stores compressed PS-ADPCM audio streams. It is a standard PlayStation format used across multiple titles, including God of War 1 and 2.

## Architecture & Hierarchy
The logic is completely identical to GOW2.

## Structure
- Header: `0x30` bytes long.
- Magic: `"VAGp"`
- **Big-Endian**: The `Data Size` and `Sample Rate` values must be parsed using Big-Endian byte order, distinguishing it from the Little-Endian standard used in other GOW formats.
