# TWK Format Specification (GOW1)

## Overview
The TWK (Tweak) format implements an internal Virtual File System (VFS) to configure gameplay variables, AI properties, combat states, and tunable variables.

## Architecture & Hierarchy
The logic is entirely identical to GOW2.

## Structure
- Tag: `0x71` or `0x72` (Combat file).
- The byte-stream command parser operates identically to GOW2, extracting directories and name hashes using the same `0xF0` bitmask evaluations (`0x80` = root, `0x10` = enter dir, `0x40` = exit dir, `0x20` = set data field).
