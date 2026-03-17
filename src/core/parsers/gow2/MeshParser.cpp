#include "MeshParser.h"
#include "core/Logger.h"
#include <cstring>

namespace GOW {

// VIF command unpack masks
static const uint32_t VIF_CMD_NOP    = 0x00;
static const uint32_t VIF_CMD_STCYCL = 0x01;
static const uint32_t VIF_CMD_STROW  = 0x30;
static const uint32_t VIF_CMD_MSCAL  = 0x14;

// Conversion factors for PS2 formats
static const float GSFixedPoint8 = 16.0f;
static const float GSFixedPoint24 = 4096.0f;

struct VifUnpackState {
    const uint8_t* xyzw = nullptr;
    const uint8_t* rgba = nullptr;
    const uint8_t* uv = nullptr;
    const uint8_t* norm = nullptr;

    int numXyzw = 0;
    int numRgba = 0;
    int numUv = 0;
    int numNorm = 0;
    int uvWidth = 0;

    void Clear() {
        xyzw = rgba = uv = norm = nullptr;
        numXyzw = numRgba = numUv = numNorm = 0;
        uvWidth = 0;
    }
};

static void FlushState(VifUnpackState& state, MeshPart& outPart, uint32_t materialId) {
    if (!state.xyzw || state.numXyzw == 0) return;

    int vertexCount = state.numXyzw;
    int baseIndex = (int)outPart.vertices.size();

    std::vector<bool> skipFlags;
    skipFlags.reserve(vertexCount);

    // 1. Build vertices
    for (int i = 0; i < vertexCount; ++i) {
        GpuVertex v;
        v.color = glm::vec4(1.0f);

        // XYZW (16-bit integer parts, / 16.0f)
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

        // NORM (8-bit signed)
        if (state.norm && i < state.numNorm) {
            int8_t nx = (int8_t)state.norm[i * 3 + 0];
            int8_t ny = (int8_t)state.norm[i * 3 + 1];
            int8_t nz = (int8_t)state.norm[i * 3 + 2];
            v.normal = glm::normalize(glm::vec3(float(nx)/127.0f, float(ny)/127.0f, float(nz)/127.0f));
        }

        // UV (16-bit or 32-bit / 4096.0f)
        if (state.uv && i < state.numUv) {
            if (state.uvWidth == 16) {
                int16_t u, v_coord;
                std::memcpy(&u, state.uv + i * 4 + 0, 2);
                std::memcpy(&v_coord, state.uv + i * 4 + 2, 2);
                v.uv.x = float(u) / GSFixedPoint24;
                v.uv.y = float(v_coord) / GSFixedPoint24;
            } else if (state.uvWidth == 32) {
                int32_t u, v_coord;
                std::memcpy(&u, state.uv + i * 8 + 0, 4);
                std::memcpy(&v_coord, state.uv + i * 8 + 4, 4);
                v.uv.x = float(u) / GSFixedPoint24;
                v.uv.y = float(v_coord) / GSFixedPoint24;
            }
        }

        // RGBA (8-bit unsigned)
        if (state.rgba && i < state.numRgba) {
            v.color.r = float(state.rgba[i * 4 + 0]) / 128.0f; // Note: PS2 uses 128 as full intensity
            v.color.g = float(state.rgba[i * 4 + 1]) / 128.0f;
            v.color.b = float(state.rgba[i * 4 + 2]) / 128.0f;
            v.color.a = float(state.rgba[i * 4 + 3]) / 128.0f;
        }

        outPart.vertices.push_back(v);
    }

    // 2. Build triangle lists from triangle strip + skip flags
    // PS2 logic: if skip is false, form triangle with (i-2, i-1, i)
    // IMPORTANT: It alternates winding order natively!
    for (int i = 2; i < vertexCount; ++i) {
        if (!skipFlags[i]) {
            int idx0 = baseIndex + i - 2;
            int idx1 = baseIndex + i - 1;
            int idx2 = baseIndex + i;

            // Invert winding to match OpenGL default CCW if needed
            // Actually, because of triangle strip, every odd triangle needs winding reversed
            if (i % 2 != 0) {
                outPart.indices.push_back(idx0);
                outPart.indices.push_back(idx2);
                outPart.indices.push_back(idx1);
            } else {
                outPart.indices.push_back(idx0);
                outPart.indices.push_back(idx1);
                outPart.indices.push_back(idx2);
            }
        }
    }

    outPart.materialId = materialId;
    state.Clear();
}

bool GOW2MeshParser::ParseDmaChain(const std::vector<uint8_t>& objData, uint32_t packetOffset, uint32_t dmaCount, MeshPart& outPart) {
    if (packetOffset >= objData.size()) return false;

    VifUnpackState state;

    uint32_t pos = packetOffset;
    for (uint32_t i = 0; i < dmaCount; ++i) {
        if (pos + 16 > objData.size()) break;

        // DMA Tag is 16 bytes. Inside it or after it is VIF data.
        // For simplicity, we just parse standard VIF codes assuming they are directly after or embedded.
        // Actually, DMA_TAG_REF points somewhere else, DMA_TAG_RET stops.
        // GOW2 uses very specific DMA sequences. We'll linearly scan the VIF stream for now.
        // Wait, DMA tag has address and qwc.
        uint64_t dmaTag;
        std::memcpy(&dmaTag, &objData[pos], 8);
        
        uint32_t dmaId = (dmaTag >> 28) & 0x7;
        uint32_t dmaQwc = dmaTag & 0xFFFF;
        uint32_t dmaAddr = (dmaTag >> 32) & 0xFFFFFFFF; // this is absolute offset in the chunk!

        uint32_t vifPos = pos + 8; // VIF codes often embedded in the second half of tag
        uint32_t vifEnd = pos + 16;
        
        // Scan VIF commands functionally needed in the tag
        auto parseVifStream = [&](uint32_t start, uint32_t end) {
            uint32_t p = start;
            while (p + 4 <= end && p + 4 <= objData.size()) {
                p = (p + 3) & ~3; // align 4
                if (p >= end || p >= objData.size()) break;

                uint16_t imm;
                uint8_t num, cmd;
                std::memcpy(&imm, &objData[p], 2);
                num = objData[p+2];
                cmd = objData[p+3];
                p += 4;

                if (cmd > 0x60) {
                    // UNPACK
                    uint8_t components = ((cmd >> 2) & 0x3) + 1;
                    int widthBits = 0;
                    switch (cmd & 0x3) {
                        case 0: widthBits = 32; break;
                        case 1: widthBits = 16; break;
                        case 2: widthBits = 8;  break;
                        case 3: widthBits = 4;  break;
                    }

                    uint32_t blockSize = components * ((widthBits * num) / 8);
                    if (p + blockSize > objData.size()) break;

                    const uint8_t* block = &objData[p];

                    if (widthBits == 16 && components == 4) {
                        state.xyzw = block;
                        state.numXyzw = num;
                    } else if (widthBits == 16 && components == 2) {
                        state.uv = block;
                        state.numUv = num;
                        state.uvWidth = 16;
                    } else if (widthBits == 32 && components == 2) {
                        state.uv = block;
                        state.numUv = num;
                        state.uvWidth = 32;
                    } else if (widthBits == 8 && components == 3) {
                        state.norm = block;
                        state.numNorm = num;
                    } else if (widthBits == 8 && components == 4) {
                        state.rgba = block;
                        state.numRgba = num;
                    }

                    p += blockSize;
                } else if (cmd == VIF_CMD_MSCAL) {
                    // MSCAL triggers the execution of the microprogram, meaning data is "flushed" to VU
                    FlushState(state, outPart, 0); // We set materialId in ParseObject
                } else if (cmd == VIF_CMD_STROW) {
                    p += 16; // STROW data
                }
            }
            return true;
        };

        if (dmaId == 3) { // DMA_TAG_REF
            parseVifStream(pos + 8, pos + 16); // inline
            parseVifStream(dmaAddr, dmaAddr + dmaQwc * 16); // referenced data
        } else if (dmaId == 6) { // DMA_TAG_RET
            parseVifStream(pos + 8, pos + 16);
            // End of chain
            break;
        }

        pos += 16;
    }
    
    // Final flush if any
    FlushState(state, outPart, 0);
    return true;
}

bool GOW2MeshParser::ParseObject(IFile& file, uint32_t objOffset, uint32_t objSize, MeshData& outData) {
    std::vector<uint8_t> buffer(objSize);
    file.Seek(objOffset, 0); // SEEK_SET
    size_t readCount = file.Read(buffer.data(), objSize);
    if (readCount != objSize) return false;

    // GOW2 Object Header (32 bytes)
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
    // uint8_t totalDmaProgramsCount = buffer[0x19];

    // Objects with type 0x1D (static) or 0x0E (dynamic) are meshes. Others are lines or invisible.
    if (type != 0x1D && type != 0x0E) return true; // skip this object, not an error

    MeshPart part;
    part.materialId = materialId;

    uint32_t dmaCalls = instancesCount * textureLayersCount;
    for (uint32_t i = 0; i < dmaCalls; ++i) {
        // According to gow2.go:
        // packetOffset = Object offset + 0x20 header + dmaChainIndex * dmaTagsCount * 0x10
        uint32_t packetOffset = 0x20 + i * dmaTagsCountPerPacket * 16;
        ParseDmaChain(buffer, packetOffset, dmaTagsCountPerPacket, part);
    }

    if (!part.vertices.empty()) {
        outData.parts.push_back(part);
    }

    return true;
}

std::unique_ptr<MeshData> GOW2MeshParser::Parse(IFile& file, uint32_t offset, uint32_t size) {
    if (size < 0x18) return nullptr; // Invalid header

    uint32_t magic;
    file.Seek(offset, 0);
    file.Read(&magic, 4);
    // Allow any generic PS2 GOW Mesh Magic (typically 0x0001000F, 0x0002000F, 0x8000000F)
    if ((magic & 0xFFFF) != 0x000F) {
        LOG_ERR("[GOW2MeshParser] Invalid magic: 0x%08X", magic);
        return nullptr;
    }

    uint32_t mdlCommentStart;
    file.Seek(offset + 4, 0);
    file.Read(&mdlCommentStart, 4);

    uint16_t partsCount;
    file.Seek(offset + 8, 0);
    file.Read(&partsCount, 2);

    auto meshData = std::make_unique<MeshData>();

    for (uint32_t i = 0; i < partsCount; ++i) {
        uint32_t partOffset;
        file.Seek(offset + 0x18 + i * 4, 0);
        file.Read(&partOffset, 4);

        uint32_t partEnd = mdlCommentStart;
        if (i != partsCount - 1) {
            file.Seek(offset + 0x18 + i * 4 + 4, 0);
            file.Read(&partEnd, 4);
        }
        
        uint32_t partSize = partEnd - partOffset;

        // Parse Part
        uint16_t groupsCount;
        file.Seek(offset + partOffset + 2, 0);
        file.Read(&groupsCount, 2);

        for (uint16_t j = 0; j < groupsCount; ++j) {
            uint32_t groupOffsetRelative;
            file.Seek(offset + partOffset + 4 + j * 4, 0);
            file.Read(&groupOffsetRelative, 4);
            
            uint32_t groupEndRelative = partSize;
            if (j != groupsCount - 1) {
                file.Seek(offset + partOffset + 4 + j * 4 + 4, 0);
                file.Read(&groupEndRelative, 4);
            }

            uint32_t groupOffset = partOffset + groupOffsetRelative;
            uint32_t groupSize = groupEndRelative - groupOffsetRelative;

            // Parse Group
            uint16_t objectsCount;
            file.Seek(offset + groupOffset + 4, 0);
            file.Read(&objectsCount, 2);

            for (uint16_t k = 0; k < objectsCount; ++k) {
                uint32_t objOffsetRelative;
                file.Seek(offset + groupOffset + 8 + k * 4, 0);
                file.Read(&objOffsetRelative, 4);
                
                uint32_t objEndRelative = groupSize;
                if (k != objectsCount - 1) {
                    file.Seek(offset + groupOffset + 8 + k * 4 + 4, 0);
                    file.Read(&objEndRelative, 4);
                }

                uint32_t objOffset = groupOffset + objOffsetRelative;
                uint32_t objSize = objEndRelative - objOffsetRelative;

                ParseObject(file, offset + objOffset, objSize, *meshData);
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
