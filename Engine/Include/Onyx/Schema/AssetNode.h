#pragma once
#include <Onyx/Schema/DataTypes.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace Onyx::Schema {

// â”€â”€ Node types â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
enum class NodeKind : uint8_t {
    Struct, Array, Number, Bool, String, Enum, Vector, Hex, Null
};

// â”€â”€ Base node â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// All parsed values are stored in a tree of AssetNodes.
// Each concrete subclass knows its display format.
class AssetNode {
public:
    virtual ~AssetNode() = default;

    std::string name;
    int         binaryOffset = 0;

    virtual NodeKind    Kind()         const = 0;
    virtual std::string DisplayValue() const = 0;

    // Children (for Struct/Array)
    std::vector<std::shared_ptr<AssetNode>> children;

    void AddChild(std::shared_ptr<AssetNode> child) {
        children.push_back(std::move(child));
    }

    // Find child by name (linear scan â€” fine for inspector use)
    AssetNode* Find(const char* childName) const {
        for (auto& c : children)
            if (c->name == childName) return c.get();
        return nullptr;
    }

    // Typed accessor shortcuts
    template<typename T>
    T* As() { return dynamic_cast<T*>(this); }

    template<typename T>
    const T* As() const { return dynamic_cast<const T*>(this); }
};

// â”€â”€ StructNode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class StructNode : public AssetNode {
public:
    std::string typeName;

    NodeKind    Kind()         const override { return NodeKind::Struct; }
    std::string DisplayValue() const override { return "[" + typeName + "]"; }
};

// â”€â”€ ArrayNode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class ArrayNode : public AssetNode {
public:
    std::string elementType;

    NodeKind    Kind()         const override { return NodeKind::Array; }
    std::string DisplayValue() const override {
        return "[" + std::to_string(children.size()) + " items]";
    }
};

// â”€â”€ NumberNode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class NumberNode : public AssetNode {
public:
    double      value       = 0;
    DataType    originalType = DataType::Int32;
    DisplayHint display     = DisplayHint::Default;

    NodeKind Kind() const override { return NodeKind::Number; }

    std::string DisplayValue() const override {
        if (display == DisplayHint::Hex || originalType == DataType::Key || originalType == DataType::Asset) {
            std::ostringstream ss;
            int width = (int)DataTypeSize(originalType) * 2;
            if (width == 0) width = 8;
            ss << "0x" << std::hex << std::uppercase
               << std::setfill('0') << std::setw(width)
               << (uint64_t)value;
            return ss.str();
        }
        if (originalType == DataType::Float) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(4) << (float)value;
            return ss.str();
        }
        if (originalType == DataType::Double) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << value;
            return ss.str();
        }
        // Integer types
        if (originalType == DataType::UInt64)
            return std::to_string((uint64_t)value);
        if (originalType == DataType::Int64)
            return std::to_string((int64_t)value);
        if (originalType == DataType::UInt32 || originalType == DataType::UInt16 || originalType == DataType::UInt8)
            return std::to_string((uint32_t)value);
        return std::to_string((int32_t)value);
    }

    // Convenience accessors
    int32_t   AsInt()   const { return (int32_t)value; }
    uint32_t  AsUInt()  const { return (uint32_t)value; }
    float     AsFloat() const { return (float)value; }
};

// â”€â”€ BoolNode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class BoolNode : public AssetNode {
public:
    bool value = false;

    NodeKind    Kind()         const override { return NodeKind::Bool; }
    std::string DisplayValue() const override { return value ? "true" : "false"; }
};

// â”€â”€ StringNode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class StringNode : public AssetNode {
public:
    std::string value;

    NodeKind    Kind()         const override { return NodeKind::String; }
    std::string DisplayValue() const override { return value; }
};

// â”€â”€ EnumNode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class EnumNode : public AssetNode {
public:
    uint32_t    rawValue = 0;
    std::string resolvedName;
    std::string enumType;

    NodeKind Kind() const override { return NodeKind::Enum; }

    std::string DisplayValue() const override {
        std::ostringstream ss;
        if (!resolvedName.empty())
            ss << resolvedName << " (0x"
               << std::hex << std::uppercase << std::setfill('0') << std::setw(8)
               << rawValue << ")";
        else
            ss << "0x" << std::hex << std::uppercase
               << std::setfill('0') << std::setw(8) << rawValue;
        return ss.str();
    }
};

// â”€â”€ VectorNode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class VectorNode : public AssetNode {
public:
    float       x = 0, y = 0, z = 0, w = 0;
    int         componentCount = 3; // 2, 3, or 4
    DisplayHint display = DisplayHint::Default;

    NodeKind Kind() const override { return NodeKind::Vector; }

    std::string DisplayValue() const override {
        auto fmt = [](float v) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(4) << v;
            return ss.str();
        };
        if (componentCount == 2) return "(" + fmt(x) + ", " + fmt(y) + ")";
        if (componentCount == 3) return "(" + fmt(x) + ", " + fmt(y) + ", " + fmt(z) + ")";
        return "(" + fmt(x) + ", " + fmt(y) + ", " + fmt(z) + ", " + fmt(w) + ")";
    }
};

// â”€â”€ HexNode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class HexNode : public AssetNode {
public:
    std::vector<uint8_t> data;

    NodeKind    Kind()         const override { return NodeKind::Hex; }
    std::string DisplayValue() const override {
        std::ostringstream ss;
        size_t n = std::min(data.size(), (size_t)32);
        for (size_t i = 0; i < n; i++)
            ss << std::hex << std::setfill('0') << std::setw(2) << (int)data[i] << " ";
        if (data.size() > 32) ss << "...";
        return ss.str();
    }
};

// â”€â”€ NullNode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class NullNode : public AssetNode {
public:
    NodeKind    Kind()         const override { return NodeKind::Null; }
    std::string DisplayValue() const override { return "(null)"; }
};

} // namespace Onyx::Schema
