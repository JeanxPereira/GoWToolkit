# Turns the two extracted TSVs into the generated header
# Source/core/formats/gowr/SmSchemaTable.h.
#
#   python tools/dump_smschema.java  (inside Ghidra)  -> smschema_fields.tsv
#   python tools/smschema_names.py GoWR.exe smschema_fields.tsv \
#          smschema_names.tsv
#   python tools/gen_smschema_header.py smschema_fields.tsv \
#          smschema_names.tsv Source/core/formats/gowr/SmSchemaTable.h
#
# Records with an implausible owner id, size or offset are dropped HERE rather
# than in the dumper, so the TSV stays a faithful record of what was read from
# the binary and every filtering decision stays reviewable in one place.

import sys, collections

FIELDS = sys.argv[1] if len(sys.argv) > 1 else "smschema_fields.tsv"
NAMES = sys.argv[2] if len(sys.argv) > 2 else "smschema_names.tsv"
DST = sys.argv[3] if len(sys.argv) > 3 else "Source/core/formats/gowr/SmSchemaTable.h"

rows = []
for i, l in enumerate(open(FIELDS, encoding="utf-8")):
    if i == 0:
        continue
    p = l.rstrip("\n").split("\t")
    if len(p) < 10:
        continue
    try:
        r = dict(addr=int(p[0], 16), name=p[1], off=int(p[2]), size=int(p[3]),
                 type=int(p[4]), owner=int(p[5]), fid=int(p[7]))
    except ValueError:
        continue
    if r["owner"] > 0x2000 or r["size"] > 0x4000 or r["off"] > 0x8000:
        continue
    if len(r["name"]) < 2:
        continue
    if not all(32 <= ord(c) < 127 for c in r["name"]):
        continue
    rows.append(r)
rows.sort(key=lambda r: r["addr"])

# ownerStructId is library-local: it restarts per registered library, so the
# same id names a different struct in each. Address order preserves library
# order and the id is non-decreasing within one, so a drop marks a boundary.
# Grouping on the id alone merged every library's struct 170 into a single
# impossible 3000-field struct whose field names repeated.
groups, lib, prev = collections.OrderedDict(), 0, -1
for r in rows:
    if r["owner"] < prev:
        lib += 1
    prev = r["owner"]
    groups.setdefault((lib, r["owner"]), []).append(r)

for k, fs in groups.items():
    seen, out = set(), []
    for f in fs:
        sig = (f["name"], f["off"], f["size"], f["type"])
        if sig in seen:
            continue
        seen.add(sig)
        out.append(f)
    out.sort(key=lambda f: (f["off"], f["addr"]))
    groups[k] = out

names = {}
for i, l in enumerate(open(NAMES, encoding="utf-8")):
    if i == 0:
        continue
    p = l.rstrip("\n").split("\t")
    if len(p) < 7 or not p[2]:
        continue
    names[(int(p[0]), int(p[1]))] = (p[2], int(p[3]))

total = sum(len(v) for v in groups.values())
distinct = {r["name"] for r in rows}
libcount = lib + 1

BS = chr(92)
QT = chr(34)


def esc(s):
    return s.replace(BS, BS + BS).replace(QT, BS + QT)


HDR = '''#pragma once

// smschema reflection tables, extracted from GoWR.exe.
//
// God of War Ragnarok describes its serialisable data with an in-house
// reflection system its own error strings name "smschema" (Sony Santa Monica
// schema -- see the path in that message: Shared/DataLayer/LibCore/
// core_library_info.cpp). The executable carries the whole thing as static
// data: every field of every schema struct, with byte offset, size and type,
// and -- in a second table -- the type names.
//
// -- Field record layout in the binary (32 bytes) --------------------------
//
//   +0x00  u64  namePtr          field name, as a C string
//   +0x08  u64  namePtr          repeated -- this doubling identifies a record
//   +0x10  u16  fieldOffset      byte offset within the owning struct
//   +0x12  u16  size             field size in bytes
//   +0x14  u16  typeCode         see SmType below
//   +0x16  u16  ownerStructId    which struct the field belongs to
//   +0x1A  u16  fieldId          global field index
//
// -- ownerStructId is LIBRARY-LOCAL ----------------------------------------
//
// The id restarts per registered library, so the same number names a different
// struct in each one and a struct is only identified by the PAIR
// (library, id). Grouping on the id alone -- which an earlier revision of this
// header did -- merges every library's struct 170 into one impossible
// 3000-field struct whose field names repeat. Records are laid out in address
// order and the id is non-decreasing within a library, so a drop in id marks
// the boundary.
//
// -- Where the struct names come from --------------------------------------
//
// Not from the field record, which never carries one. A second table in .data
// holds one type descriptor per registered type:
//
//   -0x3a  u16   count of the type's OWN fields
//   -0x2c  u32   runtime size of the C++ struct
//   +0x00  u64   -> "namespace.TypeName"
//   +0x28  u64   -> field entries, 56 bytes each, name pointer at +0x10
//
// Nothing links a descriptor to an ownerStructId, so tools/smschema_names.py
// joins the two on the field NAMES both sides carry. It is an exact-name join,
// not a similarity score: every field name in the descriptor must appear in
// the struct, and a struct with no descriptor stays unnamed.
//
// The join has to allow for the two sides describing different LAYOUTS of the
// same type. The descriptor is the runtime C++ struct; this field table is the
// serialised form, which adds a TemplateSymbol and an "<X>_IsNull" companion
// per optional field, so dctools.Fog holds LightColor at +80 here but at +96
// there. Byte offsets therefore cannot join them -- only names can.
//
// Coverage is partial and deliberately so. The descriptors present in the
// image are overwhelmingly level_scripting and behavior_tree types, whose
// fields barely appear in this table, while every dctools descriptor matches:
// 2143 of its 2144 field names are here. What is named is named exactly; the
// rest is addressed by (library, id).
//
// Extraction is reproducible: tools/dump_smschema.java walks the record shape
// above and writes the field TSV, tools/smschema_names.py reads the type
// descriptors straight out of the PE (no Ghidra, no running game), and this
// script generates the header. Regenerate when the game patches.
//
// GENERATED -- do not edit by hand.
'''

with open(DST, "w", encoding="utf-8", newline="\r\n") as f:
    w = f.write
    w(HDR)
    w("// Fields: %d   Structs: %d   Libraries: %d   Named: %d   Distinct field names: %d\n"
      % (total, len(groups), libcount, len(names), len(distinct)))
    w("// Source: GoWR.exe (PE x86-64, image base 0x140000000)\n\n")
    w("#include <cstdint>\n\n")
    w("namespace Onyx::Gowr::SmSchema {\n\n")
    w('''// Type codes as they appear in the table. The meanings are inferred from
// field names and sizes across all records rather than from the game's own
// code, so the ones marked (?) fit every observation but are unconfirmed.
enum class SmType : uint16_t {
    Int32      = 0x0000,   // also enums stored 4 wide
    Float      = 0x0008,   // by far the most common
    Ref        = 0x0010,   // handle/reference to another object (?)
    Bool       = 0x0014,
    IsNullFlag = 0x0016,   // the "<Field>_IsNull" companion of an optional
    StringHash = 0x0018,   // string stored as an 8-byte hash, NOT as text
    Array      = 0x001C,   // (?)
    Struct     = 0x0024,   // (?) embedded struct
    Embedded   = 0x0028,   // (?)
    Vector     = 0x002C,   // colour/vector; size is reported as 0
    Expression = 0x0030,   // (?)
    Enum8      = 0x0104,
    Enum8b     = 0x0105,
    Enum16     = 0x0204,
};

struct Field {
    const char* name;
    uint16_t    offset;
    uint16_t    size;
    uint16_t    type;
    uint16_t    fieldId;
};

struct Struct {
    uint16_t     library;      // which registered library the id belongs to
    uint16_t     id;           // ownerStructId, unique only within the library
    uint16_t     fieldCount;
    uint32_t     runtimeSize;  // size of the runtime C++ struct; 0 if unnamed
    const char*  name;         // "namespace.TypeName", or nullptr if unnamed
    const Field* fields;
};

''')
    for (l_, o), fs in groups.items():
        w("inline constexpr Field kFields_L%04X_S%04X[] = {\n" % (l_, o))
        for x in fs:
            w('    {"%s", %d, %d, 0x%04X, %d},\n'
              % (esc(x["name"]), x["off"], x["size"], x["type"], x["fid"]))
        w("};\n\n")
    w("inline constexpr Struct kStructs[] = {\n")
    for (l_, o), fs in groups.items():
        nm, sz = names.get((l_, o), (None, 0))
        w('    {%d, 0x%04X, %d, %d, %s, kFields_L%04X_S%04X},\n'
          % (l_, o, len(fs), sz,
             ('"%s"' % esc(nm)) if nm else "nullptr", l_, o))
    w("};\n\n")
    w("inline constexpr int kStructCount = %d;\n" % len(groups))
    w("inline constexpr int kLibraryCount = %d;\n\n" % libcount)
    w('''// The struct carrying a material constant override. It is the same shape the
// MAT entry's parameter table stores per parameter (see MaterialParser.h's
// MatParam): a name and a float value. MaterialConstantName is a StringHash,
// which is exactly why the WAD holds a nameHash and no text. This one has no
// descriptor in the image, so it is addressed by (library, id).
inline constexpr uint16_t kMaterialConstantLibrary = 250;
inline constexpr uint16_t kMaterialConstantStructId = 0x0269;

// Linear lookups over the tables above. All return nullptr when not found.
const Struct* FindStruct(uint16_t library, uint16_t id);
const Struct* FindStructByName(const char* name);
const Field*  FindField(uint16_t library, uint16_t id, const char* name);

} // namespace Onyx::Gowr::SmSchema
''')

print("wrote %s" % DST)
print("fields=%d structs=%d libraries=%d named=%d names=%d"
      % (total, len(groups), libcount, len(names), len(distinct)))
