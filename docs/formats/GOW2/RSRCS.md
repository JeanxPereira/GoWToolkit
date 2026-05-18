# RSRCS Format Specification (GOW2)

## Overview
The RSRCS (Resources) format specifies external `.WAD` dependencies required by the current archive. It essentially acts as a linking table telling the engine which files must be pre-loaded into memory to satisfy references (like cross-archive textures or models).

## Architecture & Hierarchy
The file is an extremely simple flat array of strings.

## Header Structure
There is no "Magic" header. The entire payload belongs to Tag ID `500`.

The payload consists entirely of 24-byte strings.
The total number of external WAD references is calculated as:
```c
NumWads = PayloadSize / 24;
```

Each 24-byte block represents the name of the required WAD.

## Usage
If `WAD_A` contains a `MAT` that refers to a texture inside `WAD_B`, `WAD_A` will include an `RSRCS` tag pointing to `WAD_B`. The engine parses `RSRCS` during the load phase to fetch the dependencies.
