// Animation parser — port of god_of_war_browser/pack/wad/anm
// Handles magic 0x00000003, works for both GOW1 and GOW2.

#include "AnimationParser.h"
#include "core/Logger.h"
#include <cstring>
#include <functional>
#include <string>

namespace GOW {

// Hex dump helper for format analysis
static void hexDump(const char* label, const uint8_t* data, size_t len, size_t maxBytes = 128) {
    if (!data || len == 0) return;
    size_t n = (len < maxBytes) ? len : maxBytes;
    std::string hex;
    for (size_t i = 0; i < n; ++i) {
        char tmp[4];
        snprintf(tmp, sizeof(tmp), "%02X ", data[i]);
        hex += tmp;
        if ((i & 15) == 15 && i + 1 < n) hex += "| ";
    }
    LOG_INFO("[HexDump] %s (%zu bytes): %s", label, len, hex.c_str());
}

// ── Safe buffer helpers ────────────────────────────────────────────────────

struct SafeBuf {
    const uint8_t* data;
    size_t size;

    SafeBuf() : data(nullptr), size(0) {}
    SafeBuf(const uint8_t* d, size_t s) : data(d), size(s) {}

    // Create a sub-buffer starting at offset
    SafeBuf sub(size_t off) const {
        if (off >= size) return SafeBuf(data + size, 0);
        return SafeBuf(data + off, size - off);
    }

    bool valid(size_t off, size_t len = 1) const {
        return off + len <= size;
    }

    uint32_t u32(size_t off) const {
        if (!valid(off, 4)) return 0;
        uint32_t v; memcpy(&v, data + off, 4); return v;
    }

    uint16_t u16(size_t off) const {
        if (!valid(off, 2)) return 0;
        uint16_t v; memcpy(&v, data + off, 2); return v;
    }

    int16_t s16(size_t off) const {
        if (!valid(off, 2)) return 0;
        int16_t v; memcpy(&v, data + off, 2); return v;
    }

    int8_t s8(size_t off) const {
        if (!valid(off, 1)) return 0;
        return (int8_t)data[off];
    }

    uint8_t u8(size_t off) const {
        if (!valid(off, 1)) return 0;
        return data[off];
    }

    float f32(size_t off) const {
        if (!valid(off, 4)) return 0.0f;
        float v; memcpy(&v, data + off, 4); return v;
    }

    std::string str(size_t off, size_t maxLen) const {
        if (!valid(off, 1)) return "";
        size_t avail = std::min(maxLen, size - off);
        size_t len = 0;
        while (len < avail && data[off + len] != 0) ++len;
        return std::string(reinterpret_cast<const char*>(data + off), len);
    }
};

// ── DataBitMap ─────────────────────────────────────────────────────────────

static const uint8_t kDefaultDataBitMap[] = {1, 1, 0, 0, 1, 0, 0, 0};

static DataBitMap NewDataBitMapFromBuf(const SafeBuf& b) {
    DataBitMap dbm;
    dbm.pairedElementsCount = b.u8(1);
    dbm.dataOffset = b.u16(2);
    uint8_t bitmapCount = b.u8(0);
    dbm.bitmap.resize(bitmapCount);
    for (int i = 0; i < bitmapCount; ++i) {
        dbm.bitmap[i] = b.u16(4 + i * 2);
    }
    return dbm;
}

static uint16_t bitmaskZeroBitsShift(uint16_t bitmask) {
    int32_t b = (int32_t)bitmask;
    return (uint16_t)(((b ^ (b & (-b))) << 16) >> 16);
}

static int trailingZeros16(uint16_t v) {
    if (v == 0) return 16;
    int n = 0;
    while ((v & 1) == 0) { v >>= 1; ++n; }
    return n;
}

using BitMapCallback = std::function<void(int bitIndex, int iteration)>;

static void IterateDataBitMap(const DataBitMap& dbm, const BitMapCallback& f) {
    int iteration = 0;
    for (int iWord = 0; iWord < (int)dbm.bitmap.size(); ++iWord) {
        uint16_t bitmask = dbm.bitmap[iWord];
        while (bitmask != 0) {
            int bitIndex = trailingZeros16(bitmask);
            bitmask = bitmaskZeroBitsShift(bitmask);
            f(iWord * 16 + bitIndex, iteration);
            ++iteration;
        }
    }
}

// ── Shift to coefficient ───────────────────────────────────────────────────

static float shiftToCoeff(int8_t shift) {
    if (shift == 0) return 1.0f;
    if (shift <= 0) return (float)(1 << (uint32_t)(-shift));
    return 1.0f / (float)(1 << (uint32_t)shift);
}

// ── BitMap + Shifts helpers ────────────────────────────────────────────────

static SafeBuf getDataBitMapOffset(const AnimStateDescrHeader& descr,
                                     const SafeBuf& stateData) {
    if (descr.flagsProbably & 2)
        return stateData;
    return SafeBuf(kDefaultDataBitMap, sizeof(kDefaultDataBitMap));
}

static DataBitMap GetDataBitMap(const AnimStateDescrHeader& descr,
                                const SafeBuf& stateData) {
    return NewDataBitMapFromBuf(getDataBitMapOffset(descr, stateData));
}

static std::vector<int8_t> GetShiftsArray(const AnimStateDescrHeader& descr,
                                           const SafeBuf& stateData) {
    DataBitMap dbm = GetDataBitMap(descr, stateData);
    std::vector<int8_t> shifts(dbm.pairedElementsCount);
    if (dbm.pairedElementsCount == 1) {
        // Arithmetic shift right of signed byte. Go does
        //   shifts[0] = int8(descr.FlagsProbably) >> 4
        // which sign-extends the high nibble. C++ must cast to signed FIRST,
        // otherwise (uint8 >> 4) drops the sign bit and we lose ~65536x on
        // any state whose flags byte has bit 7 set (e.g. 0xC2 → wanted -4, got 12).
        shifts[0] = (int8_t)((int8_t)descr.flagsProbably >> 4);
    } else {
        SafeBuf s5Buf = getDataBitMapOffset(descr, stateData).sub(dbm.bitmap.size() * 2 + 4);
        for (int i = 0; i < (int)dbm.pairedElementsCount; ++i) {
            shifts[i] = s5Buf.s8(i);
        }
    }
    return shifts;
}

// ── Parse rotation frames (raw) ────────────────────────────────────────────

static void parseFramesRotationRaw(const SafeBuf& buf, AnimSubstream& sub,
                                    const DataBitMap& bitMap,
                                    const AnimStateDescrHeader& descr,
                                    bool useAdditionalOffset) {
    sub.isAdditive = false;
    int additionalOffset = 0;
    if (useAdditionalOffset) {
        additionalOffset = ((int)descr.howMany64kbSkip << 16) + (int)sub.manager.offsetToData;
    }
    const int elementSize = 2;

    IterateDataBitMap(bitMap, [&](int bitIndex, int iteration) {
        std::vector<float> frames(sub.manager.count);
        int frameStep = (int)bitMap.pairedElementsCount * elementSize;
        for (int iFrame = 0; iFrame < (int)sub.manager.count; ++iFrame) {
            int offset = frameStep * iFrame + (int)bitMap.dataOffset
                       + iteration * elementSize + additionalOffset;
            frames[iFrame] = (float)buf.s16(offset);
        }
        sub.samples[(int)descr.baseTargetDataIndex + bitIndex] = std::move(frames);
    });
}

// ── Parse rotation frames (additive) ───────────────────────────────────────

static void parseFramesRotationAdd(const SafeBuf& buf, AnimSubstream& sub,
                                    const DataBitMap& bitMap,
                                    const AnimStateDescrHeader& descr,
                                    bool useAdditionalOffset,
                                    const std::vector<int8_t>& shifts) {
    sub.isAdditive = true;
    int additionalOffset = 0;
    if (useAdditionalOffset) {
        additionalOffset = ((int)descr.howMany64kbSkip << 16) + (int)sub.manager.offsetToData;
    }
    const int elementSize = 1;

    IterateDataBitMap(bitMap, [&](int bitIndex, int iteration) {
        std::vector<float> frames(sub.manager.count);
        int frameStep = (int)bitMap.pairedElementsCount * elementSize;
        for (int iFrame = 0; iFrame < (int)sub.manager.count; ++iFrame) {
            int offset = frameStep * iFrame + (int)bitMap.dataOffset
                       + iteration * elementSize + additionalOffset;
            int8_t shiftIdx = (iteration < (int)shifts.size()) ? shifts[iteration] : 0;
            frames[iFrame] = (float)buf.s8(offset) * shiftToCoeff(shiftIdx);
        }
        sub.samples[(int)descr.baseTargetDataIndex + bitIndex] = std::move(frames);
    });
}

// ── Parse position frames (raw) ────────────────────────────────────────────

static void parseFramesPositionRaw(const SafeBuf& buf, AnimSubstream& sub,
                                    const DataBitMap& bitMap,
                                    const AnimStateDescrHeader& descr,
                                    bool useAdditionalOffset) {
    sub.isAdditive = false;
    int additionalOffset = 0;
    if (useAdditionalOffset) {
        additionalOffset = ((int)descr.howMany64kbSkip << 16) + (int)sub.manager.offsetToData;
    }
    const int elementSize = 4;

    IterateDataBitMap(bitMap, [&](int bitIndex, int iteration) {
        std::vector<float> frames(sub.manager.count);
        int frameStep = (int)bitMap.pairedElementsCount * elementSize;
        for (int iFrame = 0; iFrame < (int)sub.manager.count; ++iFrame) {
            int offset = frameStep * iFrame + (int)bitMap.dataOffset
                       + iteration * elementSize + additionalOffset;
            frames[iFrame] = buf.f32(offset);
        }
        sub.samples[(int)descr.baseTargetDataIndex + bitIndex] = std::move(frames);
    });
}

// ── Parse position frames (additive) ───────────────────────────────────────

static void parseFramesPositionAdd(const SafeBuf& buf, AnimSubstream& sub,
                                    const DataBitMap& bitMap,
                                    const AnimStateDescrHeader& descr,
                                    bool useAdditionalOffset,
                                    const std::vector<int8_t>& shifts) {
    sub.isAdditive = true;
    int additionalOffset = 0;
    if (useAdditionalOffset) {
        additionalOffset = ((int)descr.howMany64kbSkip << 16) + (int)sub.manager.offsetToData;
    }
    const int elementSize = 2;

    IterateDataBitMap(bitMap, [&](int bitIndex, int iteration) {
        std::vector<float> frames(sub.manager.count);
        int frameStep = (int)bitMap.pairedElementsCount * elementSize;
        for (int iFrame = 0; iFrame < (int)sub.manager.count; ++iFrame) {
            int offset = frameStep * iFrame + (int)bitMap.dataOffset
                       + iteration * elementSize + additionalOffset;
            int8_t shiftIdx = (iteration < (int)shifts.size()) ? shifts[iteration] : 0;
            frames[iFrame] = (float)buf.s16(offset) * shiftToCoeff(shiftIdx) / 256.0f;
        }
        sub.samples[(int)descr.baseTargetDataIndex + bitIndex] = std::move(frames);
    });
}

// ── Parse rotations for a skinning state ───────────────────────────────────
// Exact port of AnimState0Skinning.ParseRotations from type0.go

static bool parseSkinningRotations(const SafeBuf& buf, int stateIndex,
                                    SkinningState& ss) {
    SafeBuf stateBuf = buf.sub(stateIndex * 0xc);
    if (!stateBuf.valid(0, 0xc)) return false;

    // Read state descriptor header
    ss.header.baseTargetDataIndex = stateBuf.u16(0);
    ss.header.flagsProbably = stateBuf.u8(2);
    ss.header.howMany64kbSkip = stateBuf.u8(3);

    // Read rotation stream manager
    ss.rotationStream.manager.count        = stateBuf.u16(4);
    ss.rotationStream.manager.offset       = stateBuf.u16(6);
    ss.rotationStream.manager.datasCount3  = stateBuf.u16(8);
    ss.rotationStream.manager.offsetToData = stateBuf.u16(10);

    size_t stateDataOff = ((uint32_t)ss.header.howMany64kbSkip << 16)
                         + (uint32_t)ss.rotationStream.manager.offsetToData;
    SafeBuf stateData = stateBuf.sub(stateDataOff);

    if (ss.rotationStream.manager.count == 0) {
        // Substream mode
        uint8_t addCount = stateData.u8(0);
        uint8_t totalSubCount = stateData.u8(1);
        SafeBuf subArrayBuf = stateData.sub(2);
        SafeBuf bitMapOffset = stateData.sub(2 + (int)totalSubCount * 8);

        DataBitMap rotBitMap = GetDataBitMap(ss.header, bitMapOffset);

        // Additive substreams
        ss.rotationSubStreamsAdd.resize(addCount);
        for (int i = 0; i < addCount; ++i) {
            auto& sub = ss.rotationSubStreamsAdd[i];
            sub.manager.count        = subArrayBuf.u16(i * 8 + 0);
            sub.manager.offset       = subArrayBuf.u16(i * 8 + 2);
            sub.manager.datasCount3  = subArrayBuf.u16(i * 8 + 4);
            sub.manager.offsetToData = subArrayBuf.u16(i * 8 + 6);
            
            if (sub.manager.count > 4096) return false;

            auto shifts = GetShiftsArray(ss.header, bitMapOffset);
            parseFramesRotationAdd(stateBuf, sub, rotBitMap, ss.header, true, shifts);
        }

        // Rough substreams
        int roughCount = std::max(0, (int)totalSubCount - (int)addCount);
        ss.rotationSubStreamsRough.resize(roughCount);
        for (int i = (int)addCount; i < (int)totalSubCount; ++i) {
            auto& sub = ss.rotationSubStreamsRough[i - addCount];
            sub.manager.count        = subArrayBuf.u16(i * 8 + 0);
            sub.manager.offset       = subArrayBuf.u16(i * 8 + 2);
            sub.manager.datasCount3  = subArrayBuf.u16(i * 8 + 4);
            sub.manager.offsetToData = subArrayBuf.u16(i * 8 + 6);

            if (sub.manager.count > 4096) return false;

            parseFramesRotationRaw(stateBuf, sub, rotBitMap, ss.header, true);
        }
    } else {
        // Single stream mode
        if (ss.rotationStream.manager.count > 4096) return false;
        
        DataBitMap rotBitMap = GetDataBitMap(ss.header, stateData);
        if ((ss.header.flagsProbably & 1) == 0) {
            parseFramesRotationRaw(stateData, ss.rotationStream, rotBitMap, ss.header, false);
        } else {
            auto shifts = GetShiftsArray(ss.header, stateData);
            parseFramesRotationAdd(stateData, ss.rotationStream, rotBitMap, ss.header, false, shifts);
        }
    }
    return true;
}

// ── Parse positions for a skinning state ───────────────────────────────────
// Exact port of AnimState0Skinning.ParsePositions from type0.go
// GOW1: posBase was at rawAct[0x80] (sd[1].OffsetToData with stride 0x14)
// GOW2: posBase at rawAct[0x78] (sd[1].OffsetToData with stride 0x10)
// Now passed explicitly to avoid hardcoded offset dependency.

static bool parseSkinningPositions(const SafeBuf& buf, int stateIndex,
                                    SkinningState& ss, const SafeBuf& rawAct,
                                    uint32_t posBase) {
    SafeBuf stateBuf = rawAct.sub(posBase + stateIndex * 0xc);
    if (!stateBuf.valid(0, 0xc)) return false;

    ss.header.baseTargetDataIndex = stateBuf.u16(0);
    ss.header.flagsProbably = stateBuf.u8(2);
    ss.header.howMany64kbSkip = stateBuf.u8(3);

    ss.positionStream.manager.count        = stateBuf.u16(4);
    ss.positionStream.manager.offset       = stateBuf.u16(6);
    ss.positionStream.manager.datasCount3  = stateBuf.u16(8);
    ss.positionStream.manager.offsetToData = stateBuf.u16(10);

    size_t stateDataOff = ((uint32_t)ss.header.howMany64kbSkip << 16)
                         + (uint32_t)ss.positionStream.manager.offsetToData;
    SafeBuf stateData = stateBuf.sub(stateDataOff);

    if (ss.positionStream.manager.count == 0) {
        uint8_t addCount = stateData.u8(0);
        uint8_t totalSubCount = stateData.u8(1);
        SafeBuf subArrayBuf = stateData.sub(2);
        SafeBuf bitMapOffset = stateData.sub(2 + (int)totalSubCount * 8);

        DataBitMap posBitMap = GetDataBitMap(ss.header, bitMapOffset);

        ss.positionSubStreamsAdd.resize(addCount);
        for (int i = 0; i < addCount; ++i) {
            auto& sub = ss.positionSubStreamsAdd[i];
            sub.manager.count        = subArrayBuf.u16(i * 8 + 0);
            sub.manager.offset       = subArrayBuf.u16(i * 8 + 2);
            sub.manager.datasCount3  = subArrayBuf.u16(i * 8 + 4);
            sub.manager.offsetToData = subArrayBuf.u16(i * 8 + 6);

            auto shifts = GetShiftsArray(ss.header, bitMapOffset);
            parseFramesPositionAdd(stateBuf, sub, posBitMap, ss.header, true, shifts);
        }

        int roughCount = std::max(0, (int)totalSubCount - (int)addCount);
        ss.positionSubStreamsRough.resize(roughCount);
        for (int i = (int)addCount; i < (int)totalSubCount; ++i) {
            auto& sub = ss.positionSubStreamsRough[i - addCount];
            sub.manager.count        = subArrayBuf.u16(i * 8 + 0);
            sub.manager.offset       = subArrayBuf.u16(i * 8 + 2);
            sub.manager.datasCount3  = subArrayBuf.u16(i * 8 + 4);
            sub.manager.offsetToData = subArrayBuf.u16(i * 8 + 6);

            parseFramesPositionRaw(stateBuf, sub, posBitMap, ss.header, true);
        }
    } else {
        DataBitMap posBitMap = GetDataBitMap(ss.header, stateData);
        if ((ss.header.flagsProbably & 1) == 0) {
            parseFramesPositionRaw(stateData, ss.positionStream, posBitMap, ss.header, false);
        } else {
            auto shifts = GetShiftsArray(ss.header, stateData);
            parseFramesPositionAdd(stateData, ss.positionStream, posBitMap, ss.header, false, shifts);
        }
    }
    return true;
}

// ── Main parser ────────────────────────────────────────────────────────────

std::unique_ptr<AnimationData> GOW2AnimationParser::Parse(const uint8_t* data, size_t size) {
    SafeBuf buf(data, size);

    if (size < 0x18) {
        LOG_WARN("[AnimParser] Data too small (%zu bytes)", size);
        return nullptr;
    }

    auto anim = std::make_unique<AnimationData>();

    uint16_t dataTypeCount = buf.u16(0x10);
    uint16_t groupCount    = buf.u16(0x12);

    anim->rawFlags = buf.u32(8);
    anim->parsedFlags.autoplay          = (anim->rawFlags & 0x1) != 0;
    anim->parsedFlags.jointRotAnimated  = (anim->rawFlags & 0x1000) != 0;
    anim->parsedFlags.jointPosAnimated  = (anim->rawFlags & 0x2000) != 0;
    anim->parsedFlags.jointScaleAnimated = (anim->rawFlags & 0x4000) != 0;

    LOG_INFO("[AnimParser] flags=0x%08X, dataTypes=%d, groups=%d, bufSize=%zu",
             anim->rawFlags, dataTypeCount, groupCount, size);

    // Hex dump: animation buffer header (first 0x20 bytes)
    hexDump("AnimHeader[0x00-0x1F]", data, std::min(size, (size_t)0x20));
    // Hex dump: around the dataType area
    size_t dtOff = 0x18 + (size_t)groupCount * 4;
    if (dtOff < size) {
        hexDump("DataTypeArea", data + dtOff, std::min(size - dtOff, (size_t)32));
    }

    // Debug: log data type IDs
    for (int i = 0; i < dataTypeCount; ++i) {
        size_t off = 0x18 + (size_t)groupCount * 4 + i * 4;
        LOG_INFO("[AnimParser] dataType[%d]: typeId=%d, param1=%d, param2=%d (off=0x%zx, raw=0x%08X)",
                 i, buf.u16(off), buf.u8(off+2), buf.u8(off+3), off, buf.u32(off));
    }

    // Parse data types (at offset 0x18 + groupCount * 4)
    anim->dataTypes.resize(dataTypeCount);
    size_t rawFormatsOff = 0x18 + (size_t)groupCount * 4;
    for (int i = 0; i < dataTypeCount; ++i) {
        size_t off = rawFormatsOff + i * 4;
        anim->dataTypes[i].typeId = buf.u16(off);
        anim->dataTypes[i].param1 = buf.u8(off + 2);
        anim->dataTypes[i].param2 = buf.u8(off + 3);
    }

    // Parse groups
    anim->groups.resize(groupCount);

    for (int ig = 0; ig < groupCount; ++ig) {
        auto& group = anim->groups[ig];
        uint32_t groupOffset = buf.u32(0x18 + ig * 4);

        // Bounds check
        if (groupOffset >= size) {
            LOG_WARN("[AnimParser] Group %d offset 0x%x out of bounds", ig, groupOffset);
            continue;
        }

        SafeBuf rawGroup = buf.sub(groupOffset);

        group.name = rawGroup.str(0x14, 0x18);  // 24 chars max
        group.isExternal = (rawGroup.u32(8) & 0x20000) != 0;

        if (group.isExternal) continue;

        uint32_t actCount = rawGroup.u32(0xc);
        if (actCount > 1000) {
            LOG_WARN("[AnimParser] Group '%s' has suspicious act count %u, skipping",
                     group.name.c_str(), actCount);
            continue;
        }
        group.acts.resize(actCount);

        for (uint32_t ia = 0; ia < actCount; ++ia) {
            auto& act = group.acts[ia];
            uint32_t actOffset = rawGroup.u32(0x30 + ia * 4);
            
            if (actOffset < 0x30) {
                // Dummy act: offset points into group header (sentinel for "idle/no action").
                // GOW engine uses act[0] as null state — real clips start at index 1.
                act.name = "";
                act.duration = 0.0f;
                LOG_INFO("[AnimParser]   Act[%u] DUMMY (offset=0x%X < 0x30), skipped", ia, actOffset);
                continue;
            }

            // rawAct is relative to rawGroup start (same as Go: rawGroup[actOffset:])
            SafeBuf rawAct = rawGroup.sub(actOffset);
            if (rawAct.size < 0x64) {
                LOG_WARN("[AnimParser] Act %d rawAct too small (off=0x%x, avail=%zu)", ia, actOffset, rawAct.size);
                continue;
            }

            act.unkFloat0x4 = rawAct.f32(0x4);
            act.unkFloat0xc = rawAct.f32(0xc);
            // GOW2: duration moved from 0x1C to 0x14
            // Evidence: attBrutalSlash @0x14 = 0x3FA22222 = 1.2666s (38 frames * 0.0333)
            //           @0x1C = 0x0002FFFF (~0.0, garbage)
            act.duration    = rawAct.f32(0x14);
            act.name = rawAct.str(0x24, 0x18);  // 24 chars max

            // ── Diagnostic: validate act header ──
            if (ig <= 1 && ia <= 2) {
                LOG_INFO("[AnimParser] g[%d].a[%d] actOff=0x%X name='%s' dur=%.4f"
                         " f0x4=%.4f f0xc=%.4f dur@0x14=%.4f dur@0x1C_old=%.6f",
                         ig, ia, actOffset, act.name.c_str(), act.duration,
                         act.unkFloat0x4, act.unkFloat0xc,
                         rawAct.f32(0x14), rawAct.f32(0x1c));
                // Hex dump: full act header (0x00-0x9F)
                char lbl[64];
                snprintf(lbl, sizeof(lbl), "g[%d].a[%d] rawAct", ig, ia);
                hexDump(lbl, rawAct.data, std::min(rawAct.size, (size_t)0xA0), 0xA0);
            }

            // Parse state descriptors (one per data type)
            // GOW2: stride is 0x10 (16 bytes), NOT 0x14 (20) like GOW1
            // Evidence: with 0x10 stride, sd[1] gets valid OffsetToData/FrameTime
            //   sd[0] @0x64: rotation skinning config
            //   sd[1] @0x74: position skinning config
            //   (sd[2] @0x84 and sd[3] @0x94 may exist for scale/other)
            constexpr size_t SD_STRIDE = 0x10;
            constexpr size_t SD_BASE   = 0x64;
            act.stateDescrs.resize(dataTypeCount);
            for (int isd = 0; isd < dataTypeCount; ++isd) {
                auto& sd = act.stateDescrs[isd];
                size_t sdOff = SD_BASE + (size_t)isd * SD_STRIDE;

                if (!rawAct.valid(sdOff, SD_STRIDE)) break;

                // ── GOW2 AnimActStateDescr binary layout (0x10 bytes) ──
                //   0x00 (u16): Unk0 (baseTargetDataIndex)
                //   0x02 (u16): CountOfSomething
                //   0x04 (u32): OffsetToData
                //   0x08 (f32): FrameTime
                //   0x0C (u32): extra field (varies per descriptor)

                sd.countOfSomething = rawAct.u16(sdOff + 0x2);
                uint32_t sdOffsetToData = rawAct.u32(sdOff + 0x4);
                sd.frameTime = rawAct.f32(sdOff + 0x8);

                // ── Diagnostic: hex dump the 0x10-byte state descriptor ──
                if (ig <= 1 && ia <= 2) {
                    char sdLbl[80];
                    snprintf(sdLbl, sizeof(sdLbl), "g[%d].a[%d].sd[%d] @sdOff=0x%zX", ig, ia, isd, sdOff);
                    hexDump(sdLbl, rawAct.data + sdOff, SD_STRIDE, SD_STRIDE);

                    // Log each field with both hex and interpreted values
                    LOG_INFO("[AnimParser] sd[%d] FIELDS: Unk0=0x%04X  CountOfSomething=%d"
                             "  OffsetToData=0x%08X  FrameTime=%f (raw=0x%08X)"
                             "  [0xC]=0x%08X  [0x10]=0x%08X",
                             isd,
                             rawAct.u16(sdOff + 0x0),
                             sd.countOfSomething,
                             sdOffsetToData,
                             sd.frameTime, rawAct.u32(sdOff + 0x8),
                             rawAct.u32(sdOff + 0xc),
                             rawAct.u32(sdOff + 0x10));

                    // Log the extra field at +0x0C
                    LOG_INFO("[AnimParser] sd[%d] extra@0xC=0x%08X",
                             isd, rawAct.u32(sdOff + 0xc));
                }

                // Validate FrameTime — should be ~1/30 (0.0333) or ~1/60 (0.0167)
                if (sd.frameTime <= 0.0f || sd.frameTime > 1.0f) {
                    LOG_WARN("[AnimParser] sd[%d] frameTime=%.6f looks invalid, clamping to 1/30",
                             isd, sd.frameTime);
                    sd.frameTime = 1.0f / 30.0f;
                }

                uint16_t dType = anim->dataTypes[isd].typeId;
                LOG_INFO("[AnimParser]   sd[%d] dType=%d countOfSomething=%d offsetToData=0x%X frameTime=%.6f",
                         isd, dType, sd.countOfSomething, sdOffsetToData, sd.frameTime);
                if (dType == 0 || dType == 1) {
                    // skinData starts at rawAct[sd.OffsetToData:]
                    SafeBuf skinData = rawAct.sub(sdOffsetToData);

                    // Go iterates sd.countOfSomething skinning states for this descriptor
                    if (sd.countOfSomething > 1000) sd.countOfSomething = 0; // Sanity check
                    
                    for (int iSkin = 0; iSkin < (int)sd.countOfSomething; ++iSkin) {
                        SkinningState ss;
                        if (!parseSkinningRotations(skinData, iSkin, ss)) break;
                        sd.skinningStates.push_back(std::move(ss));
                    }

                    // GOW2: position data comes from the SECOND state descriptor (sd[1])
                    // at stride 0x10 from sd[0]. In GOW1, posCount was at rawAct[0x7A]
                    // but that offset now falls inside sd[1] garbage with wrong stride.
                    size_t posSdOff = SD_BASE + 1 * SD_STRIDE; // sd[1] position descriptor
                    uint16_t posCount = 0;
                    uint32_t posOffsetToData = 0;
                    if (rawAct.valid(posSdOff, SD_STRIDE)) {
                        posCount = rawAct.u16(posSdOff + 0x2);
                        posOffsetToData = rawAct.u32(posSdOff + 0x4);
                        float posFT = rawAct.f32(posSdOff + 0x8);
                        LOG_INFO("[AnimParser]     PosSd: count=%d offsetToData=0x%X frameTime=%.4f",
                                 posCount, posOffsetToData, posFT);
                    }
                    // GOW2: position state base = sd[1].OffsetToData (at 0x78)
                    // GOW1 hardcoded rawAct[0x80] which is now sd[1]'s extra field (=0)
                    SafeBuf posSkinData = rawAct.sub(posOffsetToData);
                    for (int iPos = 0; iPos < (int)posCount; ++iPos) {
                        SkinningState ss;
                        if (!parseSkinningPositions(posSkinData, iPos, ss, rawAct, posOffsetToData)) break;
                        sd.skinningStates.push_back(std::move(ss));
                    }

                    size_t rotSamples = 0, posSamples = 0;
                    for (const auto& ss : sd.skinningStates) {
                        rotSamples += ss.rotationStream.samples.size();
                        for (auto& s : ss.rotationSubStreamsAdd) rotSamples += s.samples.size();
                        for (auto& s : ss.rotationSubStreamsRough) rotSamples += s.samples.size();

                        posSamples += ss.positionStream.samples.size();
                        for (auto& s : ss.positionSubStreamsAdd) posSamples += s.samples.size();
                        for (auto& s : ss.positionSubStreamsRough) posSamples += s.samples.size();
                    }

                    LOG_INFO("[AnimParser]     Skinning: states=%zu rotStreams=%zu, posStreams=%zu, frameTime=%.4f",
                             sd.skinningStates.size(), rotSamples, posSamples, sd.frameTime);
                }
            }

            LOG_INFO("[AnimParser]   Act '%s' duration=%.3f", act.name.c_str(), act.duration);
        }

        LOG_INFO("[AnimParser] Group '%s': %d acts", group.name.c_str(), (int)group.acts.size());
    }

    LOG_INFO("[AnimParser] Done: %d groups, %d total acts",
             (int)anim->groups.size(), anim->TotalActs());

    return anim;
}

} // namespace GOW
