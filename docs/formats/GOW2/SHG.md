# SHG Format Specification (GOW2)

## Overview
The SHG (Shadow LOD) format manages simple proxy geometry and bounding volumes used specifically for casting low-cost shadows or defining highly simplified LODs.

## Architecture & Hierarchy
The file starts with a header naming the Shadow LOD, followed by pointers to an array of objects containing bounding vectors.

## Header Structure

| Offset | Size | Type | Name | Description |
|--------|------|------|------|-------------|
| 0x00   | 4    | u32  | Magic| Identifier (`0x00000027`) |
| 0x04   | 12   | char | Name | Shadow LOD name string |
| 0x10   | 4    | u32  | Objects Cnt | Number of bounding objects |
| 0x14   | 4    | u32  | Offsets Table| Pointer to the offset table for parsing the objects |

## Data Payloads
An offset table exists at `Offsets Table`, containing `Objects Cnt` `u32` absolute offsets pointing to each Object.

### Object
Each object defines bounding boxes and potentially shadow volume planes.

| Offset | Size | Type | Name | Description |
|--------|------|------|------|-------------|
| 0x00   | 16   | f32[4]| Vector 1 | Bounding calculation vector |
| 0x10   | 2    | u16  | Unk | Unknown |
| 0x12   | 2    | u16  | Vec3 Cnt| Number of Vectors 3 |
| 0x14   | 2    | u16  | Vec4 Cnt| Number of Vectors 4 |
| 0x20   | 16   | f32[4]| Vector 2 | Secondary bounding vector |
| 0x30   | ...  | ...  | ... | Raw arrays of Vectors3 and Vectors4 |
