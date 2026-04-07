#include "MeshParser.h"
#include "core/Logger.h"
#include <cstring>
#include <algorithm>
#include <vector>

namespace GOW {

// VIF command constants (matching ps2/vif/vif.go)
static const uint32_t VIF_CMD_NOP    = 0x00;
static const uint32_t VIF_CMD_STCYCL = 0x01;
static const uint32_t VIF_CMD_STROW  = 0x30;
static const uint32_t VIF_CMD_MSCAL  = 0x14;

// Conversion factors for PS2 formats (matching dmaVif.go)
static const float GSFixedPoint8 = 16.0f;
static const float GSFixedPoint24 = 4096.0f;

// VU buffer base addresses used to detect VertexMeta vs Boundaries
// Matches dmaVif.go: var unpackBuffersBases = []uint32{0, 0x155, 0x2ab}
static const uint32_t VU_BUFFER_BASES[] = { 0x000, 0x155, 0x2ab };

struct VifUnpackState {
    const uint8_t* xyzw = nullptr;
    const uint8_t* rgba = nullptr;
    const uint8_t* uv = nullptr;
    const uint8_t* norm = nullptr;
    const uint8_t* vertexMeta = nullptr;  // 16-byte blocks with joint mapping info
    const uint8_t* boundaries = nullptr;  // bounding sphere per packet

    int numXyzw = 0;
    int numRgba = 0;
    int numUv = 0;
    int numNorm = 0;
    int numVertexMeta = 0;  // count of 16-byte blocks
    int numBoundaries = 0;
    int uvWidth = 0;        // 2 = 16-bit, 4 = 32-bit (matches Go uvWidth field)

    void Clear() {
        xyzw = rgba = uv = norm = vertexMeta = boundaries = nullptr;
        numXyzw = numRgba = numUv = numNorm = numVertexMeta = numBoundaries = 0;
        uvWidth = 0;
    }
};

// Extract per-block joint indices, matching Go's getJointIndexesFromMetaBlock()
// Returns [jointId0, jointId1] from the 16-byte vertex meta block
static void GetJointIndexesFromMetaBlock(const uint8_t* block, uint16_t& j0, uint16_t& j1) {
    j0 = block[13] >> 4;   // byte 13, upper nibble
    j1 = block[12] >> 2;   // byte 12, shifted right 2
}

static void FlushState(VifUnpackState& state, MeshPart& outPart) {
    if (!state.xyzw || state.numXyzw == 0) return;

    int vertexCount = state.numXyzw;
    int baseIndex = (int)outPart.vertices.size();

    std::vector<bool> skipFlags;
    skipFlags.reserve(vertexCount);

    // 1. Build vertices — positions, weights, normals, UVs, colors
    for (int i = 0; i < vertexCount; ++i) {
        GpuVertex v;
        v.color = glm::vec4(1.0f);

        // XYZW (16-bit signed integer / GSFixedPoint8) — matches dmaVif.go ToPacket()
        int16_t x, y, z;
        uint16_t flags;
        std::memcpy(&x, state.xyzw + i * 8 + 0, 2);
        std::memcpy(&y, state.xyzw + i * 8 + 2, 2);
        std::memcpy(&z, state.xyzw + i * 8 + 4, 2);
        std::memcpy(&flags, state.xyzw + i * 8 + 6, 2);

        v.position.x = float(x) / GSFixedPoint8;
        v.position.y = float(y) / GSFixedPoint8;
        v.position.z = float(z) / GSFixedPoint8;

        bool skip = (flags & 0x8000) != 0;
        skipFlags.push_back(skip);

        // Weight: lower 15 bits / GSFixedPoint24
        // Go: packet.Trias.Weight[i] = float32(int(flags&0x7fff)) / GSFixedPoint24
        float w1 = float(flags & 0x7fff) / GSFixedPoint24;
        v.boneWeights = glm::vec4(w1, 1.0f - w1, 0.0f, 0.0f);

        // NORM (8-bit signed, / 127.0f) — matches dmaVif.go ToPacket()
        if (state.norm && i < state.numNorm) {
            int8_t nx = (int8_t)state.norm[i * 3 + 0];
            int8_t ny = (int8_t)state.norm[i * 3 + 1];
            int8_t nz = (int8_t)state.norm[i * 3 + 2];
            v.normal = glm::vec3(float(nx)/127.0f, float(ny)/127.0f, float(nz)/127.0f);
            float len = glm::length(v.normal);
            if (len > 0.01f) v.normal /= len;
        }

        // UV — matches dmaVif.go ToPacket() uvWidth handling
        if (state.uv && i < state.numUv) {
            if (state.uvWidth == 2) {
                // 16-bit signed / GSFixedPoint24
                int16_t u, v_coord;
                std::memcpy(&u, state.uv + i * 4 + 0, 2);
                std::memcpy(&v_coord, state.uv + i * 4 + 2, 2);
                v.uv.x = float(u) / GSFixedPoint24;
                v.uv.y = float(v_coord) / GSFixedPoint24;
            } else if (state.uvWidth == 4) {
                // 32-bit signed / GSFixedPoint24
                int32_t u, v_coord;
                std::memcpy(&u, state.uv + i * 8 + 0, 4);
                std::memcpy(&v_coord, state.uv + i * 8 + 4, 4);
                v.uv.x = float(u) / GSFixedPoint24;
                v.uv.y = float(v_coord) / GSFixedPoint24;
            }
        }

        // RGBA (8-bit unsigned) — matches dmaVif.go ToPacket()
        if (state.rgba && i < state.numRgba) {
            v.color.r = float(state.rgba[i * 4 + 0]) / 128.0f;
            v.color.g = float(state.rgba[i * 4 + 1]) / 128.0f;
            v.color.b = float(state.rgba[i * 4 + 2]) / 128.0f;
            v.color.a = std::min(1.0f, float(state.rgba[i * 4 + 3]) / 128.0f);
        }

        outPart.vertices.push_back(v);
    }

    // 2. Decode VertexMeta blocks for Joint Indices mapping
    // Full port of dmaVif.go ToPacket() — including stitch joint handling.
    //
    // The VertexMeta block format (16 bytes per block):
    //   block[0]  = affected vertex count
    //   block[1]  = 0x80 if last block, else 0
    //   block[5]  = flags: bit5 (0x20) = stitch mode, bit4 (0x10) = push/pop direction
    //   block[12] = jointId1 * 4 (shift right 2 to get index)
    //   block[13] = jointId2 * 16 (shift right 4 to get index)
    if (state.vertexMeta && state.numVertexMeta > 0) {
        // Build blocks array (like Go's blocks [][]byte)
        int numBlocks = state.numVertexMeta;
        std::vector<const uint8_t*> blocks(numBlocks);
        for (int i = 0; i < numBlocks; ++i) {
            blocks[i] = state.vertexMeta + i * 16;
        }

        int vertIdx = 0;
        int stichPushIndex = 0;

        for (int iBlock = 0; iBlock < numBlocks; ++iBlock) {
            const uint8_t* block = blocks[iBlock];
            int blockVertCount = block[0];

            // Get base joint indices for this block
            uint16_t blockJoint0, blockJoint1;
            GetJointIndexesFromMetaBlock(block, blockJoint0, blockJoint1);

            for (int j = 0; j < blockVertCount && (baseIndex + vertIdx) < (int)outPart.vertices.size(); ++j) {
                uint16_t currentJoint0 = blockJoint0;
                uint16_t currentJoint1 = blockJoint1;

                // Stitch joint handling (exact port of Go dmaVif.go:312-329)
                if (block[5] & 0x20) {
                    if (block[5] & 0x10) {
                        // Inside stitch: odd vertices use next block's joints
                        if (stichPushIndex % 2 != 0 && (iBlock + 1) < numBlocks) {
                            GetJointIndexesFromMetaBlock(blocks[iBlock + 1], currentJoint0, currentJoint1);
                        }
                        stichPushIndex++;
                    } else {
                        // Next stitch: use previous block's joints
                        stichPushIndex--;
                        if (stichPushIndex % 2 != 0 && iBlock > 0) {
                            GetJointIndexesFromMetaBlock(blocks[iBlock - 1], currentJoint0, currentJoint1);
                        }
                    }
                }

                auto& v = outPart.vertices[baseIndex + vertIdx];
                v.boneIndices.x = currentJoint0;
                v.boneIndices.y = currentJoint1;
                vertIdx++;
            }
        }
    }

    // NOTE: We do NOT remap joint indices here.
    // The Go project stores local joint indices in the packet data and only
    // remaps via JointMappers at the point of consumption (GLTF/FBX export).
    // Our shader will do: uJoints[jointMap[boneIndices.x]] — the remap happens
    // in the shader or in ComputeJointPalette. See SceneRenderer for details.

    LOG_INFO("[FlushState] Flushing %d verts (xyzw) → building triangles", vertexCount);

    // Exact port of Go's triangle strip reconstruction (export_fbx.go).
    //
    // Critical rules:
    //   1. All vertices are stored regardless of skip flag.
    //   2. A triangle is only emitted when skip=false AND i >= 2.
    //   3. `flip` toggles on every triangle EMISSION (not every vertex).
    //   4. `flip` is NEVER reset on skip=true.
    //      Resetting flip on skip was the old bug: it caused wrong winding
    //      for all strips after the first skip vertex.
    //
    // This applies equally to MDL-direct and Object/Map paths since both
    // call the same FlushState via ParseDmaChain.
    bool flip = false;
    for (int i = 0; i < vertexCount; ++i) {
        if (!skipFlags[i]) {
            if (i >= 2) {
                int cur = baseIndex + i;
                if (flip) {
                    outPart.indices.push_back(cur - 2);
                    outPart.indices.push_back(cur - 1);
                    outPart.indices.push_back(cur);
                } else {
                    outPart.indices.push_back(cur - 1);
                    outPart.indices.push_back(cur - 2);
                    outPart.indices.push_back(cur);
                }
            }
            flip = !flip;
            // NOTE: no flip reset here — skip only suspends emission
        }
    }

    state.Clear();
}

// Parse VIF command stream from allData[start..end]
// Full port of dmaVif.go ParseVif() — handles UNPACK classification
// using VIF target address and signed flag.
static bool ParseVifStream(const std::vector<uint8_t>& allData, uint32_t start, uint32_t end,
                            VifUnpackState& state, MeshPart& outPart) {
    uint32_t p = start;
    while (p + 4 <= end && p + 4 <= allData.size()) {
        p = (p + 3) & ~3u; // align to 4 bytes
        if (p >= end || p >= allData.size()) break;

        // Read VIF code: [imm:16][num:8][cmd:8] (Go: vif.NewCode)
        uint16_t imm;
        uint8_t num, cmd;
        std::memcpy(&imm, &allData[p], 2);
        num = allData[p+2];
        cmd = allData[p+3];
        p += 4;

        if (cmd > 0x60) {
            // UNPACK command — matches dmaVif.go ParseVif() unpack branch
            uint8_t components = ((cmd >> 2) & 0x3) + 1;
            int widthBits;
            switch (cmd & 0x3) {
                case 0: widthBits = 32; break;
                case 1: widthBits = 16; break;
                case 2: widthBits = 8;  break;
                case 3: widthBits = 4;  break;
                default: widthBits = 32; break;
            }

            uint32_t blockSize = components * ((widthBits * (uint32_t)num) / 8);
            if (p + blockSize > allData.size()) break;

            const uint8_t* block = &allData[p];

            // VIF signed flag: bit 14 of IMM, 0 = signed, 1 = unsigned
            // Go: vifIsSigned := (vifCode.Imm()>>14)&1 == 0
            bool vifIsSigned = ((imm >> 14) & 1) == 0;

            // VIF target address (lower 10 bits of IMM)
            // Go: vifTarget := uint32(vifCode.Imm() & 0x3ff)
            uint32_t vifTarget = imm & 0x3ff;

            // Check if target is one of the VU buffer bases (VertexMeta indicator)
            // Go: unpackBuffersBases = []uint32{0, 0x155, 0x2ab}
            bool isBufferBase = (vifTarget == 0x000 || vifTarget == 0x155 || vifTarget == 0x2ab);

            // Classification logic — exact port of dmaVif.go ParseVif()
            switch (widthBits) {
            case 32:
                if (vifIsSigned) {
                    if (components == 4) {
                        // V4-32 signed: VertexMeta or Boundaries based on target address
                        if (isBufferBase) {
                            state.vertexMeta = block;
                            state.numVertexMeta = num;
                        } else {
                            state.boundaries = block;
                            state.numBoundaries = num;
                        }
                    } else if (components == 2) {
                        // V2-32 signed: UV with 32-bit width
                        state.uv = block;
                        state.numUv = num;
                        state.uvWidth = 4;  // Go uses uvWidth=4 for 32-bit
                    }
                }
                break;

            case 16:
                if (vifIsSigned) {
                    if (components == 4) {
                        // V4-16 signed: XYZW position data
                        state.xyzw = block;
                        state.numXyzw = num;
                    } else if (components == 2) {
                        // V2-16 signed: UV with 16-bit width
                        state.uv = block;
                        state.numUv = num;
                        state.uvWidth = 2;  // Go uses uvWidth=2 for 16-bit
                    }
                }
                break;

            case 8:
                if (components == 3 && vifIsSigned) {
                    // V3-8 signed: Normals
                    state.norm = block;
                    state.numNorm = num;
                } else if (components == 4) {
                    // V4-8: RGBA (can appear twice for multi-layer — Go accepts duplicate)
                    state.rgba = block;
                    state.numRgba = num;
                }
                break;
            }

            p += blockSize;
        } else if (cmd == VIF_CMD_MSCAL) {
            // MSCAL triggers VU microprogram execution — flush accumulated data
            FlushState(state, outPart);
        } else if (cmd == VIF_CMD_STROW) {
            p += 16; // STROW data: 4 x 32-bit values
        }
        // VIF_CMD_NOP, VIF_CMD_STCYCL: no data, just move on
    }
    return true;
}

// allData = entire mesh blob, objectOffset = object's position within mesh
// packetOffset = absolute position of DMA tags within allData
bool GOW2MeshParser::ParseDmaChain(const std::vector<uint8_t>& allData, uint32_t objectOffset,
                                    uint32_t packetOffset, uint32_t dmaCount, MeshPart& outPart) {
    if (packetOffset >= allData.size()) return false;

    VifUnpackState state;

    uint32_t pos = packetOffset;
    for (uint32_t i = 0; i < dmaCount; ++i) {
        if (pos + 16 > allData.size()) break;

        // Read DMA tag (64-bit) — matches ps2/dma/dma.go
        uint64_t dmaTag;
        std::memcpy(&dmaTag, &allData[pos], 8);

        uint32_t dmaId  = (dmaTag >> 28) & 0x7;          // Tag ID
        uint32_t dmaQwc = dmaTag & 0xFFFF;               // Quadword count
        uint32_t dmaAddr = (dmaTag >> 32) & 0x7FFFFFFF;  // 31-bit address

        if (dmaId == 3) { // DMA_TAG_REF
            // Inline VIF in tag's second qword (bytes 8-15)
            ParseVifStream(allData, pos + 8, pos + 16, state, outPart);

            // Referenced data: addr is relative to object start (matches Go: dmaPack.Addr() + ms.Object.Offset)
            uint32_t refStart = dmaAddr + objectOffset;
            uint32_t refEnd = refStart + dmaQwc * 16;
            if (refStart < allData.size() && refEnd <= allData.size()) {
                ParseVifStream(allData, refStart, refEnd, state, outPart);
            } else {
                LOG_WARN("[ParseDmaChain] REF out of bounds: addr=0x%X objOff=0x%X refStart=0x%X refEnd=0x%X bufSize=0x%zX",
                         dmaAddr, objectOffset, refStart, refEnd, allData.size());
            }
        } else if (dmaId == 6) { // DMA_TAG_RET
            // Inline VIF in tag's second qword
            ParseVifStream(allData, pos + 8, pos + 16, state, outPart);
            break; // End of chain
        } else {
            LOG_WARN("[ParseDmaChain] Unknown DMA tag id=%u at pos=0x%X (qwc=%u addr=0x%X)", dmaId, pos, dmaQwc, dmaAddr);
        }

        pos += 16;
    }

    // Final flush if any remaining state
    FlushState(state, outPart);
    return true;
}

// allData = entire mesh blob, objOffset = object's position within mesh
bool GOW2MeshParser::ParseObject(const std::vector<uint8_t>& allData, uint32_t objOffset, uint32_t objSize, MeshData& outData) {
    if (objOffset + objSize > allData.size()) return false;
    const uint8_t* buffer = &allData[objOffset];

    // GOW2 Object Header (0x20 = 32 bytes) — matches gow2.go parseGow2()
    uint16_t type, unk02, materialId, jointMapCount;
    uint32_t dmaTagsCountPerPacket, instancesCount, flags, flagsMask;

    std::memcpy(&type, &buffer[0], 2);
    std::memcpy(&unk02, &buffer[2], 2);
    std::memcpy(&dmaTagsCountPerPacket, &buffer[4], 4);
    std::memcpy(&materialId, &buffer[8], 2);
    std::memcpy(&jointMapCount, &buffer[0xA], 2);
    std::memcpy(&instancesCount, &buffer[0xC], 4);
    std::memcpy(&flags, &buffer[0x10], 4);
    std::memcpy(&flagsMask, &buffer[0x14], 4);

    uint8_t textureLayersCount = buffer[0x18];
    if (textureLayersCount == 0) textureLayersCount = 1;

    LOG_INFO("[ParseObject] type=0x%04X matId=%u instances=%u layers=%u dmaPerPkt=%u jointMapCnt=%u objOff=0x%X objSize=%u",
             type, materialId, instancesCount, textureLayersCount, dmaTagsCountPerPacket, jointMapCount, objOffset, objSize);

    MeshPart basePart;
    basePart.materialId = materialId;

    uint32_t dmaCalls = instancesCount * textureLayersCount;
    // Joint map starts right after all DMA programs
    // Go: jointMapOffset := OBJECT_GOW1_HEADER_SIZE + dmaCalls*0x10*dmaTagsCountPerPacket
    uint32_t baseJointMapOffset = 0x20 + dmaCalls * 16 * dmaTagsCountPerPacket;

    for (uint32_t inst = 0; inst < instancesCount; ++inst) {
        // Parse this instance's joint map (uint32 array)
        // Go: o.JointMappers[iJm] = make([]uint32, o.JointMapElementsCount)
        std::vector<uint16_t> instJointMap;
        if (jointMapCount > 0) {
            uint32_t jointMapOffset = baseJointMapOffset + inst * jointMapCount * 4;
            if (jointMapOffset + jointMapCount * 4 <= objSize) {
                instJointMap.reserve(jointMapCount);
                for (int i = 0; i < jointMapCount; ++i) {
                    uint32_t globalJointId;
                    std::memcpy(&globalJointId, &buffer[jointMapOffset + i * 4], 4);
                    instJointMap.push_back((uint16_t)globalJointId);
                }
            }
        }

        for (uint32_t layer = 0; layer < textureLayersCount; ++layer) {
            MeshPart instPart = basePart;
            instPart.textureLayer = layer;
            instPart.jointMap = instJointMap;
            instPart.name = "Inst " + std::to_string(inst) + " Layer " + std::to_string(layer);

            uint32_t dmaIndex = inst * textureLayersCount + layer;
            // Packet offset is absolute within allData (objOffset + header + dma tag stride)
            uint32_t packetOff = objOffset + 0x20 + dmaIndex * dmaTagsCountPerPacket * 16;
            ParseDmaChain(allData, objOffset, packetOff, dmaTagsCountPerPacket, instPart);

            if (!instPart.vertices.empty()) {
                outData.parts.push_back(std::move(instPart));
            }
        }
    }

    return true;
}

std::unique_ptr<MeshData> GOW2MeshParser::Parse(IFile& file, uint32_t offset, uint32_t size) {
    if (size < 0x18) return nullptr; // Invalid header

    // Read the ENTIRE mesh blob into memory (Go does the same: allb []byte)
    std::vector<uint8_t> allData(size);
    file.Seek(offset, SEEK_SET);
    if (file.Read(allData.data(), size) != size) return nullptr;

    uint32_t magic;
    std::memcpy(&magic, &allData[0], 4);
    // Both GOW1 and GOW2 share MESH_MAGIC = 0x0001000F (see god_of_war_browser/pack/wad/mesh/mesh.go).
    // The game version determines the header format, not the magic.
    // This parser is always called from GOW2 context, so enforce GOW2 format.
    if ((magic & 0xFFFF) != 0x000F) {
        LOG_ERR("[GOW2MeshParser] Invalid magic: 0x%08X (expected lower 16 = 0x000F)", magic);
        return nullptr;
    }

    uint32_t mdlCommentStart;
    std::memcpy(&mdlCommentStart, &allData[4], 4);

    // Always use GOW2 format: 0x18-byte header, partsCount as uint16 at offset 8.
    // (GOW1 meshes use 0x50-byte header and uint32 partsCount — handled by a separate parser.)
    GameVersion version = GameVersion::GOW2;
    uint32_t partsCount = 0;
    {
        uint16_t pc16;
        std::memcpy(&pc16, &allData[8], 2);
        partsCount = pc16;
    }
    LOG_INFO("[GOW2MeshParser] mesh magic=0x%08X parts=%u mdlCommentStart=0x%X", magic, partsCount, mdlCommentStart);

    auto meshData = std::make_unique<MeshData>();
    uint32_t meshHeaderSize = (version == GameVersion::GOW1) ? 0x50 : 0x18;

    for (uint32_t i = 0; i < partsCount; ++i) {
        uint32_t partOffset;
        std::memcpy(&partOffset, &allData[meshHeaderSize + i * 4], 4);

        uint32_t partEnd = mdlCommentStart;
        if (i != partsCount - 1) {
            std::memcpy(&partEnd, &allData[meshHeaderSize + i * 4 + 4], 4);
        }

        uint32_t partSize = partEnd - partOffset;

        // Parse Part
        uint16_t groupsCount;
        std::memcpy(&groupsCount, &allData[partOffset + 2], 2);

        for (uint16_t j = 0; j < groupsCount; ++j) {
            uint32_t groupOffsetRelative;
            std::memcpy(&groupOffsetRelative, &allData[partOffset + 4 + j * 4], 4);

            uint32_t groupEndRelative = partSize;
            if (j != groupsCount - 1) {
                std::memcpy(&groupEndRelative, &allData[partOffset + 4 + j * 4 + 4], 4);
            }

            uint32_t groupOffset = partOffset + groupOffsetRelative;
            uint32_t groupSize = groupEndRelative - groupOffsetRelative;

            // Parse Group
            uint32_t objectsCount = 0;
            if (version == GameVersion::GOW1) {
                std::memcpy(&objectsCount, &allData[groupOffset + 4], 4);
            } else {
                uint16_t oc16;
                std::memcpy(&oc16, &allData[groupOffset + 4], 2);
                objectsCount = oc16;
            }

            uint32_t groupHeaderSize = (version == GameVersion::GOW1) ? 0xC : 0x8;
            
            for (uint32_t k = 0; k < objectsCount; ++k) {
                uint32_t objOffsetRelative;
                std::memcpy(&objOffsetRelative, &allData[groupOffset + groupHeaderSize + k * 4], 4);

                uint32_t objEndRelative = groupSize;
                if (k != objectsCount - 1) {
                    std::memcpy(&objEndRelative, &allData[groupOffset + groupHeaderSize + k * 4 + 4], 4);
                }

                uint32_t objOffset = groupOffset + objOffsetRelative;
                uint32_t objSize = objEndRelative - objOffsetRelative;

                ParseObject(allData, objOffset, objSize, *meshData);
            }
        }
    }

    // Compute final bounds
    glm::vec3 bmin(100000.0f);
    glm::vec3 bmax(-100000.0f);
    for (auto& part : meshData->parts) {
        for (auto& v : part.vertices) {
            bmin = glm::min(bmin, v.position);
            bmax = glm::max(bmax, v.position);
        }
    }
    meshData->bounds.min = bmin;
    meshData->bounds.max = bmax;

    LOG_INFO("[GOW2MeshParser] Parsed %zu parts.", meshData->parts.size());
    return meshData;
}

} // namespace GOW
