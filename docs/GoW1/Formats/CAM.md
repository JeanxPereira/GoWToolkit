# CAM Format Specification (GOW1)

## Overview
The CAM (Camera) format describes cinematic or in-game camera paths, rails, and behaviors. 

## Architecture & Hierarchy
The standard `CAM` node typically encapsulates a "Rail" system, representing the camera's path through space. This is structurally identical to GOW2.

## Header Structure
| Offset | Size | Type | Name | Description |
|--------|------|------|------|-------------|
| 0x00   | 4    | u32  | Count| Number of points/matrices in the camera rail |
| 0x04   | 4    | u32  | Unk04| Unknown (Usually `0`) |
| 0x08   | 4    | u32  | Unk08| Unknown (Usually `0xFFFFFFFF`) |
| 0x0C   | 4    | u32  | Unk0C| Unknown (Usually `0xFFFFFFFF`) |

Arrays of `Mat4` (16 floats) and `float32` arrays follow immediately after the header if `Count > 0`.
