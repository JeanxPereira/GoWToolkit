# SHG Format Specification (GOW1)

## Overview
The SHG (Shadow LOD) format manages simple proxy geometry and bounding volumes used specifically for casting low-cost shadows or defining highly simplified LODs.

## Architecture & Hierarchy
Structurally identical to GOW2.

## Structure
- Magic: `0x00000027`
- The file starts with a header naming the Shadow LOD, followed by pointers to an array of objects containing bounding vectors.
