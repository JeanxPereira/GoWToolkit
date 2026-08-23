#include "core/formats/gowr/SmSchema.h"

#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <unordered_set>

namespace Onyx::Gowr::SmSchema {

const char* TypeName(uint16_t typeCode) {
    switch (static_cast<SmType>(typeCode)) {
        case SmType::Int32:       return "int32";
        case SmType::Bool32:      return "bool32?";
        case SmType::Small:       return "small?";
        case SmType::RigidRef:    return "rigidRef?";
        case SmType::Float:       return "float";
        case SmType::Ref:         return "ref?";
        case SmType::Bool:        return "bool";
        case SmType::BoolFlag:    return "boolFlag?";
        case SmType::IsNullFlag:  return "isNull";
        case SmType::StringHash:  return "stringHash";
        case SmType::TemplateSym: return "templateSymbol";
        case SmType::Array:       return "array?";
        case SmType::NodeRef:     return "nodeRef?";
        case SmType::Struct:      return "struct?";
        case SmType::Embedded:    return "embedded?";
        case SmType::Vector:      return "vector";
        case SmType::Expression:  return "expression?";
        case SmType::Enum8:       return "enum8";
        case SmType::Enum8b:      return "enum8b";
        case SmType::Enum16:      return "enum16";
    }
    // Held in a rotating buffer so a caller can print a couple of unknown
    // codes in one statement without them aliasing each other.
    static thread_local char slots[4][16];
    static thread_local int next = 0;
    char* out = slots[next];
    next = (next + 1) % 4;
    std::snprintf(out, sizeof(slots[0]), "unknown(0x%04X)", typeCode);
    return out;
}

const Field* FindFieldAt(uint16_t library, uint16_t id, uint16_t offset) {
    const Struct* s = FindStruct(library, id);
    if (!s) return nullptr;
    for (uint16_t i = 0; i < s->fieldCount; ++i)
        if (s->fields[i].offset == offset) return &s->fields[i];
    return nullptr;
}

std::vector<const Struct*> FindStructsNamed(const char* name) {
    std::vector<const Struct*> out;
    if (!name) return out;
    for (const auto& s : kStructs)
        if (s.name && std::strcmp(s.name, name) == 0) out.push_back(&s);
    return out;
}

std::vector<const Struct*> FindStructsWithField(const char* fieldName) {
    std::vector<const Struct*> out;
    if (!fieldName) return out;
    for (const auto& s : kStructs) {
        for (uint16_t i = 0; i < s.fieldCount; ++i) {
            if (std::strcmp(s.fields[i].name, fieldName) == 0) {
                out.push_back(&s);
                break;
            }
        }
    }
    return out;
}

Stats GetStats() {
    Stats st;
    st.structs = kStructCount;
    st.libraries = kLibraryCount;
    std::unordered_set<std::string> names;
    for (const auto& s : kStructs) {
        if (s.name) ++st.namedStructs;
        st.fields += s.fieldCount;
        for (uint16_t i = 0; i < s.fieldCount; ++i) names.insert(s.fields[i].name);
    }
    st.distinctFieldNames = static_cast<int>(names.size());
    return st;
}

bool Validate(std::vector<std::string>& problems) {
    const size_t before = problems.size();
    auto fail = [&problems](std::string msg) { problems.push_back(std::move(msg)); };

    const size_t n = sizeof(kStructs) / sizeof(kStructs[0]);
    if (static_cast<int>(n) != kStructCount)
        fail("kStructCount (" + std::to_string(kStructCount) + ") disagrees with the "
             "array length (" + std::to_string(n) + ")");

    std::set<std::pair<uint16_t, uint16_t>> seen;
    uint16_t maxLibrary = 0;

    for (const auto& s : kStructs) {
        const std::string who = "struct (" + std::to_string(s.library) + ", 0x" +
                                std::to_string(s.id) + ")";
        if (!seen.insert({s.library, s.id}).second)
            fail(who + " appears more than once; (library, id) must be unique");
        if (s.library > maxLibrary) maxLibrary = s.library;

        if (s.fieldCount == 0)
            fail(who + " has no fields");
        if (!s.fields) {
            fail(who + " has a null field array");
            continue;
        }

        // A name and a runtime size come from the same descriptor, so one
        // without the other means the join wrote a half-result.
        if ((s.name != nullptr) != (s.runtimeSize != 0))
            fail(who + " has a name without a runtime size, or the reverse");
        if (s.name && std::strchr(s.name, '.') == nullptr)
            fail(who + " is named \"" + s.name + "\", which is not namespace.TypeName");

        uint16_t prev = 0;
        for (uint16_t i = 0; i < s.fieldCount; ++i) {
            const Field& f = s.fields[i];
            if (!f.name || f.name[0] == '\0') {
                fail(who + " field " + std::to_string(i) + " has no name");
                continue;
            }
            // The generator sorts by offset; out of order means the grouping
            // pulled in a record belonging to another struct.
            if (i > 0 && f.offset < prev)
                fail(who + " field \"" + f.name + "\" is at offset " +
                     std::to_string(f.offset) + ", behind the previous field's " +
                     std::to_string(prev));
            prev = f.offset;
        }
    }

    if (maxLibrary + 1 != kLibraryCount)
        fail("kLibraryCount is " + std::to_string(kLibraryCount) +
             " but the highest library seen is " + std::to_string(maxLibrary));

    return problems.size() == before;
}

} // namespace Onyx::Gowr::SmSchema
