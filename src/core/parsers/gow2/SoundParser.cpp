#include "SoundParser.h"
#include "core/vfs/SliceFile.h"
#include "core/Logger.h"
#include <cstring>
#include <algorithm>

namespace GOW {

static std::string ReadFixedString(const uint8_t* buf, size_t maxLen) {
    size_t len = strnlen((const char*)buf, maxLen);
    return std::string((const char*)buf, len);
}

static uint32_t ReadU32LE(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t ReadU16LE(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

std::unique_ptr<GOW2SoundParser::SoundBankData> GOW2SoundParser::Parse(
    const ParsedEntry& entry,
    const std::shared_ptr<IFile>& fileSource)
{
    if (!fileSource) {
        LOG_ERR("[GOW2Sound] fileSource is null for SFX %s", entry.name.c_str());
        return nullptr;
    }
    if (entry.size < 8) {
        LOG_ERR("[GOW2Sound] SFX %s is too small: %u bytes", entry.name.c_str(), entry.size);
        return nullptr;
    }

    // Read entire entry data
    std::vector<uint8_t> buf(entry.size);
    SliceFile slice(fileSource, entry.offset, entry.size);
    slice.Seek(0, SEEK_SET);
    size_t bytesRead = slice.Read(buf.data(), entry.size);
    if (bytesRead < 8) {
        LOG_ERR("[GOW2Sound] Failed to read SFX data for %s", entry.name.c_str());
        return nullptr;
    }

    // Header: serverId (4 bytes) + soundsCount (4 bytes)
    uint32_t serverId = ReadU32LE(buf.data());
    uint32_t soundsCount = ReadU32LE(buf.data() + 4);

    LOG_INFO("[GOW2Sound] Parsing SFX '%s': serverId=0x%X, soundsCount=%u, totalSize=%u",
             entry.name.c_str(), serverId, soundsCount, entry.size);

    if (soundsCount == 0 || soundsCount > 10000) {
        LOG_ERR("[GOW2Sound] Suspicious sound count: %u for %s", soundsCount, entry.name.c_str());
        return nullptr;
    }

    auto result = std::make_unique<SoundBankData>();

    // Sound info array: starts at offset 8, each entry is 28 bytes (24-char name + 4-byte streamId)
    size_t soundInfoStart = 8;
    size_t soundInfoSize = soundsCount * 28;
    if (soundInfoStart + soundInfoSize > buf.size()) {
        LOG_ERR("[GOW2Sound] Sound info overflows buffer for %s", entry.name.c_str());
        return nullptr;
    }

    result->sounds.resize(soundsCount);
    for (uint32_t i = 0; i < soundsCount; i++) {
        size_t off = soundInfoStart + i * 28;
        result->sounds[i].name = ReadFixedString(buf.data() + off, 24);
        result->sounds[i].streamId = ReadU32LE(buf.data() + off + 24);
    }

    // Bank data starts after the sound info array
    size_t bankStart = soundInfoStart + soundInfoSize;
    if (bankStart + 24 > buf.size()) {
        LOG_ERR("[GOW2Sound] No bank data in %s", entry.name.c_str());
        // Return what we have - sounds without ADPCM data
        return result;
    }

    // Bank info: 24 bytes
    //   offset 0x00: padding/unused (8 bytes)
    //   offset 0x08: headerBlockStart (4 bytes, relative to bankStart)
    //   offset 0x0C: headerBlockSize  (4 bytes)
    //   offset 0x10: streamBlockStart (4 bytes, relative to bankStart)
    //   offset 0x14: streamBlockSize  (4 bytes)
    uint32_t headerBlockStart = ReadU32LE(buf.data() + bankStart + 0x08);
    uint32_t headerBlockSize  = ReadU32LE(buf.data() + bankStart + 0x0C);
    uint32_t streamBlockStart = ReadU32LE(buf.data() + bankStart + 0x10);
    uint32_t streamBlockSize  = ReadU32LE(buf.data() + bankStart + 0x14);

    LOG_INFO("[GOW2Sound] Bank: headerStart=0x%X, headerSize=0x%X, streamStart=0x%X, streamSize=0x%X",
             headerBlockStart, headerBlockSize, streamBlockStart, streamBlockSize);

    // Absolute offsets within buf
    size_t absHeaderStart = bankStart + headerBlockStart;
    size_t absStreamStart = bankStart + streamBlockStart;

    // Copy stream block (raw ADPCM data)
    if (streamBlockSize > 0 && absStreamStart + streamBlockSize <= buf.size()) {
        result->bankStreamData.assign(
            buf.data() + absStreamStart,
            buf.data() + absStreamStart + streamBlockSize);
    }

    // Parse bank header to extract sound descriptors
    if (headerBlockSize < 0x40 || absHeaderStart + headerBlockSize > buf.size()) {
        LOG_ERR("[GOW2Sound] Bank header too small or overflows for %s", entry.name.c_str());
        return result;
    }

    const uint8_t* hdr = buf.data() + absHeaderStart;

    // Bank header layout (from Go code):
    //   offset 0x16: soundsCount (uint16)
    //   offset 0x20: commandsStart (uint32, relative to bank header)
    //   offset 0x28: adpcmSize (uint32)
    //   offset 0x34: smpdStart (uint32, relative to bank header)
    //   offset 0x40: sounds_info starts (each sound is 0x0C = 12 bytes)
    uint16_t bankSoundsCount = ReadU16LE(hdr + 0x16);
    uint32_t commandsStart   = ReadU32LE(hdr + 0x20);
    uint32_t smpdStart       = ReadU32LE(hdr + 0x34);

    LOG_INFO("[GOW2Sound] Bank header: bankSoundsCount=%u, commandsStart=0x%X, smpdStart=0x%X",
             bankSoundsCount, commandsStart, smpdStart);

    if (bankSoundsCount == 0 || bankSoundsCount > 10000) {
        LOG_ERR("[GOW2Sound] Suspicious bank sounds count: %u", bankSoundsCount);
        return result;
    }

    // Parse each bank sound descriptor
    // Each descriptor is 12 bytes starting at offset 0x40 in the bank header
    // Layout: B0, B1, unused(2), commandCount(1), B5, B6, unused(1), commandOffset(4)
    struct BankSoundDesc {
        uint8_t  commandCount;
        uint32_t commandOffset;
    };

    std::vector<BankSoundDesc> bankSounds(bankSoundsCount);
    size_t soundsInfoOff = 0x40;
    for (uint16_t i = 0; i < bankSoundsCount; i++) {
        size_t off = soundsInfoOff + i * 12;
        if (absHeaderStart + off + 12 > buf.size()) break;

        bankSounds[i].commandCount = hdr[off + 4];
        bankSounds[i].commandOffset = ReadU32LE(hdr + off + 8);
    }

    // Parse commands and extract SampleRef data
    // Commands start at commandsStart in the bank header, each command is 8 bytes
    // SMPD references start at smpdStart in the bank header
    // Command layout: U0 (4 bytes), U4 (4 bytes)
    //   Cmd = U0 >> 24
    //   addr = U0 & 0x00FFFFFF (offset into SMPD area relative to bank header start)
    // For Cmd=1 (SampleRef): at addr in the SMPD area:
    //   offset 16: adpcmOffset (4 bytes)
    //   offset 20: adpcmSize (4 bytes)

    for (uint16_t si = 0; si < bankSoundsCount; si++) {
        const auto& bs = bankSounds[si];
        for (uint8_t ci = 0; ci < bs.commandCount; ci++) {
            size_t cmdOff = absHeaderStart + commandsStart + bs.commandOffset + ci * 8;
            if (cmdOff + 8 > buf.size()) break;

            uint32_t u0 = ReadU32LE(buf.data() + cmdOff);
            uint8_t cmd = (uint8_t)(u0 >> 24);
            uint32_t addr = u0 & 0x00FFFFFF;

            if (cmd == 1) {
                // SampleRef at addr relative to SMPD block start (which is relative to bank header)
                size_t sampleRefOff = absHeaderStart + smpdStart + addr;
                if (sampleRefOff + 24 > buf.size()) break;

                uint32_t adpcmOffset = ReadU32LE(buf.data() + sampleRefOff + 16);
                uint32_t adpcmSize   = ReadU32LE(buf.data() + sampleRefOff + 20);

                // Dump the 24 bytes of SampleRef
                std::string dump = "";
                for (int d = 0; d < 24; ++d) {
                    char hex[4];
                    sprintf(hex, "%02X ", buf[sampleRefOff + d]);
                    dump += hex;
                }

                // Link this ADPCM data to the corresponding sound
                // Find which sound maps to this bank sound index
                for (auto& snd : result->sounds) {
                    if (snd.streamId == si && !snd.hasData) {
                        snd.adpcmOffset = adpcmOffset;
                        snd.adpcmSize = adpcmSize;
                        snd.hasData = true;
                        LOG_INFO("[GOW2Sound] Sound '%s': ADPCM offset=0x%X, size=0x%X, hex=%s",
                                 snd.name.c_str(), adpcmOffset, adpcmSize, dump.c_str());
                        break;
                    }
                }

                // Also check if no sound mapped to this index yet - create an entry
                // for bank sounds that don't have a name in the header
                bool found = false;
                for (const auto& snd : result->sounds) {
                    if (snd.streamId == si) { found = true; break; }
                }
                if (!found && adpcmSize > 0) {
                    SoundInfo info;
                    info.name = "bank_sound_" + std::to_string(si);
                    info.streamId = si;
                    info.adpcmOffset = adpcmOffset;
                    info.adpcmSize = adpcmSize;
                    info.hasData = true;
                    result->sounds.push_back(std::move(info));
                }
            }
        }
    }

    LOG_INFO("[GOW2Sound] Parsed %zu sounds, stream data size: %zu bytes",
             result->sounds.size(), result->bankStreamData.size());

    return result;
}

} // namespace GOW
