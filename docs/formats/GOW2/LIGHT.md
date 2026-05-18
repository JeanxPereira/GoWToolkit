# LIGHT Format Specification (GOW2)

## Overview
The LIGHT format dictates the parameters for dynamic or static lighting in the engine (Ambient, Point, or Directional).

## Architecture & Hierarchy
Standalone file of `0x58` bytes describing lighting properties.

## Header Structure

| Offset | Size | Type | Name | Description |
|--------|------|------|------|-------------|
| 0x00   | 4    | u32  | Magic| Identifier (`0x00000006`) |
| 0x04   | 4    | u32  | Unk04| Unknown (Usually `0`) |
| 0x08   | 4    | u32  | Flags| `0` = Ambient, `1` = Point, `2`/`6` = Directional |
| 0x0C   | 16   | f32[4]| Position | XYZ location of the light source |
| 0x1C   | 16   | f32[4]| Rotation | Rotation direction (for Directional lights) |
| 0x2C   | 16   | f32[4]| Color | RGB Color vectors. Includes negative limits / intensity values |
| 0x3C   | 28   | f32[]| Properties | Floats for falloff, radius, intensity, etc. |
