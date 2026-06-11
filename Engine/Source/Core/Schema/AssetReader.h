#pragma once
#include "AssetNode.h"
#include "DataTypes.h"
#include "../vfs/IFile.h"
#include <memory>
#include <cstring>
#include <algorithm>

namespace Onyx::Schema {

// ── AssetReader ────────────────────────────────────────────────────────────
// Turns binary data + StructDefinition into a tree of AssetNodes.
// Stateless — all methods are static.
class AssetReader {
public:

    // Parse raw bytes into a node tree using the given struct definition.
    static std::shared_ptr<StructNode> Parse(const StructDefinition& def,
                                              const uint8_t* data, size_t dataSize) {
        auto root = std::make_shared<StructNode>();
        root->name = def.name;
        root->typeName = def.name;
        root->binaryOffset = 0;

        for (const auto& f : def.fields) {
            if (f.type == DataType::Pad) continue;  // skip padding

            // Bounds check
            uint32_t fieldEnd = f.offset + f.size;
            if (fieldEnd > dataSize && f.type != DataType::HexDump && f.type != DataType::String)
                continue;

            auto node = ReadField(f, data, dataSize);
            if (node) root->AddChild(std::move(node));
        }
        return root;
    }

    // Convenience: parse from an IFile at current position.
    static std::shared_ptr<StructNode> Parse(const StructDefinition& def,
                                              std::shared_ptr<Onyx::Vfs::IFile> file) {
        if (!file) return nullptr;
        std::vector<uint8_t> buf(def.size);
        file->Seek(0, SEEK_SET);
        if (file->Read(buf.data(), def.size) != (int64_t)def.size)
            return nullptr;
        return Parse(def, buf.data(), buf.size());
    }

    // Parse from an IFile at a specific offset.
    static std::shared_ptr<StructNode> Parse(const StructDefinition& def,
                                              std::shared_ptr<Onyx::Vfs::IFile> file,
                                              int64_t offset, size_t size) {
        if (!file) return nullptr;
        size_t readSize = std::min((size_t)def.size, size);
        std::vector<uint8_t> buf(readSize);
        file->Seek(offset, SEEK_SET);
        if (file->Read(buf.data(), readSize) != (int64_t)readSize)
            return nullptr;
        return Parse(def, buf.data(), buf.size());
    }

private:
    static std::shared_ptr<AssetNode> ReadField(const FieldDef& f,
                                                 const uint8_t* data,
                                                 size_t dataSize) {
        switch (f.type) {

            // ── Bool ───────────────────────────────────────────────────
            case DataType::Bool: {
                auto n = std::make_shared<BoolNode>();
                n->name = f.name;
                n->binaryOffset = (int)f.offset;
                n->value = (f.offset < dataSize) ? (data[f.offset] != 0) : false;
                return n;
            }

            // ── Integers & Floats ──────────────────────────────────────
            case DataType::UInt8:  return MakeNumber<uint8_t>(f, data);
            case DataType::Int8:   return MakeNumber<int8_t>(f, data);
            case DataType::UInt16: return MakeNumber<uint16_t>(f, data);
            case DataType::Int16:  return MakeNumber<int16_t>(f, data);
            case DataType::UInt32: return MakeNumber<uint32_t>(f, data);
            case DataType::Int32:  return MakeNumber<int32_t>(f, data);
            case DataType::UInt64: return MakeNumber<uint64_t>(f, data);
            case DataType::Int64:  return MakeNumber<int64_t>(f, data);
            case DataType::Float:  return MakeNumber<float>(f, data);
            case DataType::Double: return MakeNumber<double>(f, data);

            // ── Key / Asset (displayed as hex uint32) ──────────────────
            case DataType::Key:
            case DataType::Asset: {
                auto n = std::make_shared<NumberNode>();
                n->name = f.name;
                n->binaryOffset = (int)f.offset;
                n->originalType = f.type;
                n->display = DisplayHint::Hex;
                n->value = (double)Read<uint32_t>(data, f.offset);
                return n;
            }

            // ── Enum ───────────────────────────────────────────────────
            case DataType::Enum: {
                auto n = std::make_shared<EnumNode>();
                n->name = f.name;
                n->binaryOffset = (int)f.offset;
                n->rawValue = Read<uint32_t>(data, f.offset);
                if (f.enumName) n->enumType = f.enumName;
                // Resolve from inline map
                if (f.enumMap) {
                    for (auto& [val, label] : *f.enumMap) {
                        if (val == n->rawValue) {
                            n->resolvedName = label;
                            break;
                        }
                    }
                }
                return n;
            }

            // ── Vectors ────────────────────────────────────────────────
            case DataType::Vec2: {
                auto n = std::make_shared<VectorNode>();
                n->name = f.name;
                n->binaryOffset = (int)f.offset;
                n->display = f.display;
                n->componentCount = 2;
                n->x = Read<float>(data, f.offset);
                n->y = Read<float>(data, f.offset + 4);
                return n;
            }
            case DataType::Vec3: {
                auto n = std::make_shared<VectorNode>();
                n->name = f.name;
                n->binaryOffset = (int)f.offset;
                n->display = f.display;
                n->componentCount = 3;
                n->x = Read<float>(data, f.offset);
                n->y = Read<float>(data, f.offset + 4);
                n->z = Read<float>(data, f.offset + 8);
                return n;
            }
            case DataType::Vec4: {
                auto n = std::make_shared<VectorNode>();
                n->name = f.name;
                n->binaryOffset = (int)f.offset;
                n->display = f.display;
                n->componentCount = 4;
                n->x = Read<float>(data, f.offset);
                n->y = Read<float>(data, f.offset + 4);
                n->z = Read<float>(data, f.offset + 8);
                n->w = Read<float>(data, f.offset + 12);
                return n;
            }

            // ── String ─────────────────────────────────────────────────
            case DataType::String: {
                auto n = std::make_shared<StringNode>();
                n->name = f.name;
                n->binaryOffset = (int)f.offset;
                size_t maxLen = (f.offset < dataSize)
                    ? std::min((size_t)f.size, dataSize - (size_t)f.offset)
                    : 0;
                if (maxLen > 0)
                    n->value = std::string(
                        (const char*)data + f.offset,
                        strnlen((const char*)data + f.offset, maxLen));
                return n;
            }

            // ── Hex Dump ───────────────────────────────────────────────
            case DataType::HexDump: {
                auto n = std::make_shared<HexNode>();
                n->name = f.name;
                n->binaryOffset = (int)f.offset;
                size_t len = (f.offset < dataSize)
                    ? std::min((size_t)f.size, dataSize - (size_t)f.offset)
                    : 0;
                if (len > 0)
                    n->data.assign(data + f.offset, data + f.offset + len);
                return n;
            }

            default:
                return nullptr;
        }
    }

    // ── Numeric helper ─────────────────────────────────────────────────
    template<typename T>
    static std::shared_ptr<NumberNode> MakeNumber(const FieldDef& f, const uint8_t* data) {
        auto n = std::make_shared<NumberNode>();
        n->name = f.name;
        n->binaryOffset = (int)f.offset;
        n->originalType = f.type;
        n->display = f.display;
        n->value = (double)Read<T>(data, f.offset);
        return n;
    }

    // ── Raw read helper ────────────────────────────────────────────────
    template<typename T>
    static T Read(const uint8_t* data, uint32_t offset) {
        T val{};
        std::memcpy(&val, data + offset, sizeof(T));
        return val;
    }
};

} // namespace Onyx::Schema
