# Turns the TSV that tools/dump_smschema.java produces into the generated
# header Source/core/formats/gowr/SmSchemaTable.h.
#
#   python tools/gen_smschema_header.py smschema_fields.tsv \
#          Source/core/formats/gowr/SmSchemaTable.h
#
# Records with an implausible owner id, size or offset are dropped HERE rather
# than in the dumper, so the TSV stays a faithful record of what was read from
# the binary and every filtering decision stays reviewable in one place.

import sys, collections

SRC = sys.argv[1] if len(sys.argv) > 1 else "smschema_fields.tsv"
DST = sys.argv[2] if len(sys.argv) > 2 else "Source/core/formats/gowr/SmSchemaTable.h"

rows = []
for i, l in enumerate(open(SRC, encoding="utf-8")):
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

rows.sort(key=lambda r: (r["owner"], r["off"], r["addr"]))
byowner = collections.OrderedDict()
for r in rows:
    byowner.setdefault(r["owner"], []).append(r)

for o, fs in byowner.items():
    seen, out = set(), []
    for f in fs:
        k = (f["name"], f["off"], f["size"], f["type"])
        if k in seen:
            continue
        seen.add(k)
        out.append(f)
    byowner[o] = out

total = sum(len(v) for v in byowner.values())
names = {r["name"] for r in rows}

BS = chr(92)
QT = chr(34)


def esc(s):
    return s.replace(BS, BS + BS).replace(QT, BS + QT)


HDR = '''#pragma once

// smschema field table, extracted from GoWR.exe.
//
// God of War Ragnarok describes its serialisable data with an in-house
// reflection system its own error strings name "smschema" (Sony Santa Monica
// schema -- see the path in that message: Shared/DataLayer/LibCore/
// core_library_info.cpp). The executable carries the entire table as static
// data: every field of every schema struct, with byte offset, size and type.
//
// -- Record layout in the binary (32 bytes) -------------------------------
//
//   +0x00  u64  namePtr          field name, as a C string
//   +0x08  u64  namePtr          repeated -- this doubling identifies a record
//   +0x10  u16  fieldOffset      byte offset within the owning struct
//   +0x12  u16  size             field size in bytes
//   +0x14  u16  typeCode         see SmType below
//   +0x16  u16  ownerStructId    which struct the field belongs to
//   +0x1A  u16  fieldId          global field index
//
// -- What is NOT here, and why --------------------------------------------
//
// Struct NAMES. The binary holds 3132 strings shaped "namespace.TypeName"
// (core.Vector3, creatureeditor.DrivenBlendNodeData, ...) in a separate
// region, but nothing found so far links ownerStructId to one of them: the id
// is not stored in the struct record, does not index that table positionally,
// and no pointer runs from a struct record into this field table. The link
// most likely lives in the library registrar -- the function that emits the
// "Too many smschema library informations are registered" error -- which is
// where to look next. Until then a struct is identified by its id.
//
// Extraction is reproducible: tools/dump_smschema.java walks the record shape
// above over 0x142000000..0x143000000 and writes the TSV this header is
// generated from. Regenerate when the game patches.
//
// GENERATED -- do not edit by hand.
'''

with open(DST, "w", encoding="utf-8", newline="\r\n") as f:
    w = f.write
    w(HDR)
    w("// Fields: %d   Structs: %d   Distinct field names: %d\n" % (total, len(byowner), len(names)))
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
    uint16_t     id;
    uint16_t     fieldCount;
    const Field* fields;
};

''')
    for o, fs in byowner.items():
        w("inline constexpr Field kFields_%04X[] = {\n" % o)
        for x in fs:
            w('    {"%s", %d, %d, 0x%04X, %d},\n'
              % (esc(x["name"]), x["off"], x["size"], x["type"], x["fid"]))
        w("};\n\n")
    w("inline constexpr Struct kStructs[] = {\n")
    for o, fs in byowner.items():
        w("    {0x%04X, %d, kFields_%04X},\n" % (o, len(fs), o))
    w("};\n\n")
    w("inline constexpr int kStructCount = %d;\n\n" % len(byowner))
    w('''// The struct carrying a material constant override. It is the same shape the
// MAT entry's parameter table stores per parameter (see MaterialParser.h's
// MatParam): a name and a float value. MaterialConstantName is a StringHash,
// which is exactly why the WAD holds a nameHash and no text.
inline constexpr uint16_t kMaterialConstantStructId = 0x0269;

// Linear lookups over the tables above. Both return nullptr when not found.
const Struct* FindStruct(uint16_t id);
const Field*  FindField(uint16_t structId, const char* name);

} // namespace Onyx::Gowr::SmSchema
''')

print("wrote %s" % DST)
print("fields=%d structs=%d names=%d" % (total, len(byowner), len(names)))
