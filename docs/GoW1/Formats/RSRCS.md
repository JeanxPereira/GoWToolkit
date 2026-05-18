# RSRCS Format Specification (GOW1)

## Overview
The RSRCS (Resources) format specifies external `.WAD` dependencies required by the current archive.

## Architecture & Hierarchy
Structurally identical to GOW2.

## Structure
- Tag: `500`
- The payload consists entirely of 24-byte strings pointing to external `.WAD` files that need to be pre-loaded into the engine's memory pool.
