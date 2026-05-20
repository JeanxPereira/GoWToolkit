# GoW2 Animation Engine — Reverse Engineering Notes

## Goal

Several Kratos animations (e.g. `deathFlyBack`, `titanSlam01`, `titanSlash02`,
`hitKneel`) render with twisted, broken poses in GoWToolkit while playing
correctly in-game. The Go reference (`god_of_war_browser`) parses these formats
but does not implement a fully accurate runtime for GOW2 either, so it cannot
be used to validate behavior.

This document records a Ghidra-driven investigation of the GOW2 PS2 binary
(`SCUS_974.81`) to locate the actual joint pose evaluator and understand how
the game converts compressed animation samples into joint matrices.

## Approach

Ghidra MCP was used to query the disassembly directly. Multiple custom Java
scripts were written and executed inside Ghidra to surface candidate
functions by combining signals (string xrefs, opcode patterns, memory access
patterns). All scripts live in `~/ghidra_scripts/` and outputs in `/tmp/`.

| Script | Output | Purpose |
|--------|--------|---------|
| `FindAnimFunctions` | `/tmp/anim_funcs.txt` | Generic anim candidates (strings + Q14 immediates + sin/cos pairs) |
| `FindAnimEngine` | `/tmp/anim_engine.txt` | Strings + 0x8000 mask + Q14 float consts |
| `AnimControllerCallees` | `/tmp/anim_callees.txt` | Callees of the anim controller, profiled by VU op density |
| `FindJointPoseWriter` | `/tmp/joint_pose_writers.txt` | Functions writing mat4 at struct +0x20 with version stamp at +0x68 |
| `FindQuatDispatch` | `/tmp/quat_dispatch.txt` | Functions with andi 0x8000 + lhu + sqc2@+0x20 |
| `FindAnimSystemUsers` | `/tmp/anim_system_users.txt` | Xrefs to AnimSystem pool at 0x335dc0 |
| `DumpTypeNames` | `/tmp/type_names.txt` | All anim-related class name strings in rdata |
| `FindAnimStringXrefs` | `/tmp/anim_str_xrefs.txt` | Xrefs to error strings like "No animation on this object" |
| `DumpAnimVTable` | `/tmp/anim_vtable.txt` | Dump runtime vtable array at 0x362c48 |

The first heuristics (Q14 immediate, 0x8000 mask) yielded many false positives
because half-float decode (s10e5 → f32 via shifts) uses identical bit
patterns. Refinement was needed to filter signal from noise.

## Joint Struct Layout

The game's joint structure was decoded from `FUN_0013c010` (world matrix
refresh) and `FUN_00143d48` (mat × mat utility):

| Offset | Type | Field |
|--------|------|-------|
| `+0x0c` | `s16` | `parent_idx` — `-1` = root, `-2` = external attachment, `-3` = special, `>= 0` = parent joint index |
| `+0x10` | `ptr` | Owner skeleton reference (resolved via `*(int*)(+4) + 0xc`) |
| `+0x14` | `u16` | Flags (joint role bits, including `0x8000` quat marker in our parsing) |
| `+0x18` | `u32` | Promoted flags (init reads u16, stores u32) |
| `+0x20..+0x60` | `mat4` | Local matrix — first slot used when `parent_idx == -1` |
| `+0x50..+0x8c` | `mat4` | Local matrix in AnimSystem instances (offset differs) |
| `+0x60` | `s16` | Parent index (joint-struct flavor) |
| `+0x68` | `u64` | Version stamp (compared against global `DAT_002d7978`) |
| `+0x70..+0xb0` | `mat4` | World matrix (parent_world × local), result of `FUN_0013c010` |
| `+0xb8..+0x140` | — | Scratch and child-list pointers |

Two related global counters:

* `DAT_002d7978` — global epoch / dirty stamp incremented when anything that
  affects joint state changes. Used as version match in lazy invalidation.
* `DAT_00362c48` — runtime-populated array of `(type_id → vtable*)` for the
  animation object dispatch table. Empty in static analysis (initialized
  late by `FUN_00278100`).

## Key Functions Located

| Address | Name | Role |
|---------|------|------|
| `FUN_0013c010` | Joint world matrix refresh | If parent ≥ 0 reads parent world, multiplies by local at +0x20, stores to +0x70. Sets version. Lazy invalidation via `DAT_002d7978`. |
| `FUN_00143d48` | Mat4 × Mat4 utility | Pure VU MAC sequence (`vmulabc`/`vmaddabc`/`vmaddbc`). Used by `FUN_0013c010`. |
| `FUN_00247bd8` | Anim command dispatcher (1410 insns, 264 BBs) | Iterates active keyframes, decodes half-float start/end times (`(uVar & 0x8000) << 16 \| (uVar & 0x7fff) << 13`) / `1.92593e-34`, dispatches by opcode at `puVar45[2]`. Calls subroutines per case. |
| `FUN_00271b00` | Translation delta applier | `out_row3 = in_row3 + (in_row0·hx + in_row1·hy + in_row2·hz) × 16`. Reads three half-floats from `[a1+0xa..0xe]`. NOT joint anim — used by navigation/physics step. |
| `FUN_0023d998` | AnimSystem instance ctor | If `*(short*)(+0xc) == -1` writes identity mat4 at +0x50..+0x8c; otherwise copies parent joint mat from skeleton (via `FUN_00143d48` refresh) at parent_idx × 0x40. |
| `FUN_0023e950` | AnimSystem evaluator (1163 lines) | Loops 3 anim slots, picks max by `size·width·depth`, reads selected joint mat from skeleton. **This is the camera/cinematic transform evaluator**, not joint skinning. |
| `FUN_001d3678` | Anim script command parser | Parses `!N` / `$var` / `name` anim references. Dispatches to vtable methods on anim objects via `(&DAT_00362c48)[type_id] + 0x20`. References `"No animation on this object"` error string. |
| `FUN_001d4078` | `SCR_JointVisibility` handler | Parses script parameters and writes joint visibility flags. References `"_JointVisibility"` error. Updates `DAT_002d7978`. |
| `FUN_00278100` | Global game init (1303 lines) | Registers all pools (`tAnimSystem`, `tMove`, `tMoveSystem`, `tHand`, `tFightSystem`, etc.) via `FUN_00181428(name) + FUN_001819e8(pool, type, size, align)`. Populates `DAT_00362c48` vtable array. |

## Type Registry (Pool Sizes)

From `FUN_00278100`, type names and instance sizes:

| Type Name | Pool Addr | Size | Notes |
|-----------|-----------|------|-------|
| `tIOSystem` | 0x335ea8 | 0x1c | |
| `tGrappleSystem` | 0x335ec0 | 0x50 | |
| `goSoldier` | 0x335bd8 | 0x300 | |
| `goPassive` | 0x335ba0 | 0x410 | |
| `goPassiveAI` | 0x335bf0 | 0x30 | |
| `goGridAI` | 0x335c08 | 0x7c | |
| **`tAnimSystem`** | **0x335dc0** | **0x110** | Camera/cinematic transform |
| `tChained` | 0x335e60 | 0x160 | |
| `tPlayerEffectSystem` | 0x335e78 | 0x2c | |
| `tStandardEffectSystem` | 0x335e90 | 0x28 | |
| `tFightSystem` | 0x335ea8 | 0x1c | |
| `tFlyingSystem` | 0x335ec0 | 0x50 | |
| `tHand` | 0x335ef0 | 0x18 | |
| **`tMove`** | **0x335f20** | **0x300** | Move state machine |
| **`tMoveSystem`** | **0x335f38** | **0xe0** | Move dispatcher |
| `tScriptInfo` | 0x335f50 | 0xc | |
| `tValidityDisk` | 0x335f78 | 0x80 | |

`tAnimSystem` turned out to manage **camera and cinematic transform slots**,
not skeletal animation. Its evaluator iterates three slots, picks the one
with the largest screen-space metric, and copies that joint's mat from the
skeleton.

`tMove` and `tMoveSystem` are the most likely remaining candidates for
skeletal motion state, but they were not exhaustively decompiled.

## Anim Object Dispatch

`FUN_001d3678` shows the dispatch pattern used to send commands to anim
objects:

```c
type_struct = (&DAT_00362c48)[anim_obj->type_id];     // type_id at puVar11[0]
vtable      = *(int*)(type_struct + 0x20);
vtable[0x34]  // start method
vtable[0x44]  // stop method
vtable[0x84]  // tick method
```

`DAT_00362c48` is populated at runtime by `FUN_00278100`, so the vtable
contents are not visible in pure static analysis.

## Animation Controller (FUN_00247bd8) — Behavior

```c
fVar49 = curTime / sampleDt;        // normalized 0..1 within range
while (active keyframe in list) {
    start_t = half_decode(kf+4);    // half-float start time
    end_t   = half_decode(kf+6);    // half-float end time
    if (fVar49 < start_t || end_t < fVar49) {
        // out of range: detach, free
    }
}

while (in current pose chain) {
    opcode = puVar45[2];
    switch (opcode) {
        case 0..6: time-range / dependency check
    }
    switch (uVar25 = *puVar45) {     // action opcode
        case 1: apply flags to param_1[0xac]
        case 2: scalar value (FUN_00216418/FUN_00213518)
        case 3: ?
        case 4: ?
        ...
    }
}
```

The controller does **NOT** decode rotation samples into joint matrices
itself. It manages keyframe lifecycle and dispatches by opcode. The actual
joint matrix production happens in:

* CPU side: per-anim-type vtable methods accessed via `DAT_00362c48`
* GPU/coprocessor side: likely VU1 microcode

## The VU1 Wall

Net findings strongly suggest joint pose evaluation runs on **VU1 microcode**,
not MIPS R5900:

* 5887 MIPS functions surveyed — zero contain a tight loop decoding
  compressed s8 samples × shift coefficients into f32 quat components and
  building a mat4 in the same body.
* VU instructions (`vmulabc`, `vmaddabc`, `vmaddbc`) appear only in
  utility routines (mat × mat, vector scale) — never in a sample-decode
  context.
* `FUN_001fb868` and siblings called before joint-touching code look like
  DMA chain setup, consistent with packaging anim data + bind for VU1
  upload.
* The pattern of half-float (s10e5) packing observed throughout is the
  data interchange format the PS2 anim system pushes onto VU1.

Architecturally this matches standard PS2 design: CPU manages anim
bookkeeping (active list, timing, transitions), VU1 runs a microcode
program that reads the compressed stream + bind pose, produces world
matrices into VU memory, and DMA pushes them to GS for skinning.

## Why Pure Ghidra Hit a Wall

To recover the actual sample → joint matrix transform we would need to:

1. Extract VU1 microcode from the binary (separate object file or embedded
   blob)
2. Disassemble with a VU1-specific tool (no Ghidra processor module ships
   with one out of the box)
3. Trace DMA chains from `FUN_001fb868`-class setup functions to identify
   which microcode program handles which anim type
4. Manually map VU register usage and infer the algorithm

That is its own multi-day project. Within MIPS-only Ghidra exploration, the
trail stops at vtable dispatch into runtime-populated tables.

## Implication for GoWToolkit

The toolkit renders on OpenGL/GLM, not VU1. The Ghidra trail tells us **how
the game engine evaluates joints**, but the toolkit needs its own
mathematically-equivalent CPU path. So reaching parity with the game
binary's instruction sequence is not required — what is required is that
the toolkit produces the same final pose given the same input data.

The parser already matches the Go reference semantically (Add vs Rough
substreams, shift coefficients, baseTargetDataIndex addressing). The
divergence with broken animations sits in **applier / runtime**:

* Bake stores per-frame Q14 vec4 from accumulated samples
* `ApplyBakedAt` originally linearly mixed those vec4 components between
  frames — replaced with slerp for quat joints (helped partially but did
  not fully fix `deathFlyBack` etc.)
* Frame-to-frame quat values are far from unit (norm ≈ 0.3 in some cases),
  so even slerp can produce non-physical intermediates.

## Diagnostic Logs Captured

For Kratos OBJ + `ANM_hero`:

```
[JointFlags] j[2] flags=0x000086A0 skinned=1 external=0 quat=1 name='pelvis'
[JointFlags] j[3] flags=0x000006A2 skinned=1 external=0 quat=0 name='vertebrae2'
[BindDiag] pelvis(j2) bindRot=(0,0,0,2607) isQuat=1
[BakeDiag] act='attBrutalSlash' states=6
[BakeDiag] act='deathFlyBack'   states=6
```

`bindRot.w = 2607` (= 0.159 in Q14) — bind quat is **NOT unit-normalized**
in raw V5 storage. After normalize → identity (`(0,0,0,1)`). The game must
either normalize at load time or operate on un-normalized quats and
normalize at final mat-build. Our applier accumulates onto raw V5 then
normalizes when converting to mat — which should be equivalent. Yet
pelvis localRot for `deathFlyBack` ends up at `(-18179, 188, 0, -33692)`
(norm = 2.34, far from unit) producing visibly wrong pose direction.

Both broken and OK animations have identical state count and bind values,
so the divergence is purely in the sample data the player consumes.

## Open Questions

1. Are the `Rough` substreams overwriting joint values that should be
   delta-accumulated? The C++ matches Go (`+=` vs `=`), but Go itself
   doesn't fully work for GOW2, so this remains suspect.
2. Are there per-state version semantics we're missing? Multiple
   `skinningStates` per act are looped sequentially — could the game's
   engine evaluate them as alternatives rather than a chain?
3. Is the s8 × shiftCoeff decode correct for ALL ranges? Negative shifts
   (small fraction) and positive shifts (large multiplier) are
   reproduced, but boundary cases were not exhaustively verified.
4. Could the issue be in how root-motion / translation accumulation
   interacts with skinned vertex transforms? The pelvis often sits at
   trans = (0, 0, 0) for keyframed-in-place anims but reflects movement
   for travel anims.

## Recommendations

Short-term:
* Stop further Ghidra exploration without a VU1 microcode extractor —
  diminishing returns confirmed.
* Validate parser-only correctness by dumping per-frame baked pelvis
  values vs Go reference output for the same WAD. Diff narrows whether
  bug is in parse or in apply.
* Consider rewriting `ApplyBakedAt` to perform proper slerp + euler
  blending **and** normalize bind quats once at load time, so accumulated
  deltas operate on identity-anchored values.

Medium-term:
* Inspect PCSX2 source code — its VU1 interpreter / recompiler must
  faithfully execute the microcode and may have community notes on GOW
  animation pipelines.
* Compare with NTSC vs PAL binaries — if differences exist they often
  highlight unstable algorithm parameters.
* Try locating publicly available GoW (engine) postmortems / GDC talks
  — the engine evolved from PS2 to PS3 and some skeletal anim docs
  exist for the later titles.

Long-term:
* Build a VU1 microcode disassembler / runner subset specifically for
  the joint anim program. Once identified by DMA chain, the program is
  finite and bounded.

## References

* Binary: `SCUS_974.81` (GOW2 PS2 NTSC)
* Anim file used for testing: `ANM_hero` (Kratos)
* Reference Go implementation: `/Users/jeanxpereira/CodingProjects/god_of_war_browser`
* Ghidra scripts: `/Users/jeanxpereira/ghidra_scripts/Find{AnimFunctions,AnimEngine,JointPoseWriter,QuatDispatch,AnimSystemUsers,AnimStringXrefs}.java`,
  `AnimControllerCallees.java`, `DumpTypeNames.java`, `DumpAnimVTable.java`
