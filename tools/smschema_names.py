# Recovers the smschema struct NAMES from GoWR.exe and pairs them with the
# struct ids in the field table that tools/dump_smschema.java extracted.
#
#   python tools/smschema_names.py "D:/.../GoWR.exe" smschema_fields.tsv \
#          smschema_names.tsv
#
# Reads the PE directly. No Ghidra, no running game: everything needed is
# static data in .data.
#
# -- How the link is made ---------------------------------------------------
#
# The field table gives ownerStructId per field but never a struct name. The
# names live in a second structure: a type descriptor, one per registered type,
# also in .data:
#
#   -0x3a  u16   number of the type's OWN fields (1..4096)
#   -0x2c  u32   runtime size of the C++ struct, in bytes
#   +0x00  u64   -> "namespace.TypeName"
#   +0x28  u64   -> field entry array, 56 bytes per entry:
#                     +0x00  u64  the field's own type descriptor
#                     +0x08  u64  byte offset in the low 16 bits, flags above
#                     +0x10  u64  -> field name
#
# Nothing statically connects a descriptor to an ownerStructId, so the two are
# joined on the field NAMES that both sides carry.
#
# -- Three things this has to get right, each of which broke an earlier try --
#
# ownerStructId is LIBRARY-LOCAL. Grouping on it alone merges every library's
# struct 170 into one impossible 3000-field struct whose field names repeat.
# Records are laid out in address order and the id is non-decreasing within a
# library, so a drop in id marks a boundary; the key is (library, id).
#
# The two sides describe DIFFERENT LAYOUTS of the same type. The descriptor is
# the runtime C++ struct; the field table is the serialised form, which adds a
# TemplateSymbol and an "<X>_IsNull" companion per optional field. dctools.Fog
# holds LightColor at +80 in the table but +96 in the descriptor. So byte
# offsets cannot join the two -- only names can, after case and underscores are
# normalised away (WetnessTintColor against wetness_tint_color).
#
# A descriptor lists only the type's OWN fields: dctools.MaterialDriver starts
# at offset 336 because everything before it belongs to its base. An inheriting
# type's descriptor can therefore only be CONTAINED in its struct, never equal.
#
# Matching is on exact field names, never on shared vocabulary. A struct with
# no descriptor stays unnamed rather than taking the closest guess.

import sys, struct, collections

EXE = sys.argv[1]
FIELDS = sys.argv[2] if len(sys.argv) > 2 else "smschema_fields.tsv"
OUT = sys.argv[3] if len(sys.argv) > 3 else "smschema_names.tsv"
MIN_CONTAINED = 3

blob = open(EXE, "rb").read()
pe = struct.unpack_from("<I", blob, 0x3C)[0]
if blob[pe:pe + 4] != b"PE\0\0" or struct.unpack_from("<H", blob, pe + 24)[0] != 0x20B:
    sys.exit("not a PE32+ image")
imgbase, = struct.unpack_from("<Q", blob, pe + 48)
nsec, = struct.unpack_from("<H", blob, pe + 6)
optsz, = struct.unpack_from("<H", blob, pe + 20)

secs, off = [], pe + 24 + optsz
for _ in range(nsec):
    nm = blob[off:off + 8].rstrip(b"\0").decode("latin1")
    vsz, va, rsz, raw = struct.unpack_from("<IIII", blob, off + 8)
    secs.append((nm, imgbase + va, raw, rsz))
    off += 40


def fo(va):
    for _, sva, raw, rsz in secs:
        if sva <= va < sva + rsz:
            return raw + (va - sva)
    return None


def cstr(va, cap=140):
    o = fo(va)
    if o is None:
        return None
    e = blob.find(b"\0", o, o + cap)
    if e < 0:
        return None
    try:
        s = blob[o:e].decode("ascii")
    except UnicodeDecodeError:
        return None
    return s if s and all(32 <= ord(c) < 127 for c in s) else None


def norm(s):
    return s.replace("_", "").replace(" ", "").lower()


def serialisation_only(n):
    return n.endswith("isnull") or n == "templatesymbol"


# ---- descriptors, found by record shape ------------------------------------
_, dsva, draw, drsz = next(s for s in secs if s[0] == ".data")
descs = []
for pos in range(0x40, drsz - 0x40, 8):
    o = draw + pos
    if struct.unpack_from("<I", blob, o + 4)[0] != 1:   # high half of a 0x1_ pointer
        continue
    tn = cstr(struct.unpack_from("<Q", blob, o)[0])
    if not tn or "." not in tn or len(tn) < 5 or " " in tn or "/" in tn:
        continue
    nf = struct.unpack_from("<H", blob, o - 0x3a)[0]
    arr = struct.unpack_from("<Q", blob, o + 0x28)[0]
    if not (0 < nf <= 4096) or not arr:
        continue
    names = []
    for i in range(nf):
        eo = fo(arr + i * 0x38)
        if eo is None:
            break
        fn = cstr(struct.unpack_from("<Q", blob, eo + 0x10)[0])
        if not fn or len(fn) < 2:
            break
        names.append(norm(fn))
    if len(names) != nf:      # a coincidental byte pattern will not yield nf names
        continue
    descs.append(dict(name=tn, nf=nf,
                      size=struct.unpack_from("<I", blob, o - 0x2c)[0],
                      key=frozenset(names)))
print("type descriptors: %d  (%d distinct names)"
      % (len(descs), len({d["name"] for d in descs})))

# ---- field table, segmented into libraries ---------------------------------
rows = []
for i, l in enumerate(open(FIELDS, encoding="utf-8")):
    if i == 0:
        continue
    p = l.rstrip("\n").split("\t")
    if len(p) < 10:
        continue
    try:
        a, o_, s_, ow = int(p[0], 16), int(p[2]), int(p[3]), int(p[5])
    except ValueError:
        continue
    if ow > 0x2000 or s_ > 0x4000 or o_ > 0x8000 or len(p[1]) < 2:
        continue
    if not all(32 <= ord(c) < 127 for c in p[1]):
        continue
    rows.append((a, p[1], ow))
rows.sort()

groups, lib, prev = collections.OrderedDict(), 0, -1
for a, nm, ow in rows:
    if ow < prev:
        lib += 1
    prev = ow
    groups.setdefault((lib, ow), []).append(nm)
print("libraries: %d   structs: %d   fields: %d" % (lib + 1, len(groups), len(rows)))

# ---- join ------------------------------------------------------------------
byexact = collections.defaultdict(list)
for d in descs:
    byexact[d["key"]].append(d)
n2d = collections.defaultdict(set)
for i, d in enumerate(descs):
    for n in d["key"]:
        n2d[n].add(i)

named, amb = {}, 0
for key, fs in groups.items():
    have = {norm(x) for x in fs}
    core = {n for n in have if not serialisation_only(n)}
    hit, how = None, None
    for k in (core, have):
        c = byexact.get(frozenset(k))
        if c and len({x["name"] for x in c}) == 1:
            hit, how = c[0], "exact"
            break
    if hit is None:
        cand = set()
        for n in core:
            cand |= n2d.get(n, set())
        fits = [i for i in cand
                if descs[i]["key"] <= core and len(descs[i]["key"]) >= MIN_CONTAINED]
        if fits:
            best = max(len(descs[i]["key"]) for i in fits)
            top = {descs[i]["name"] for i in fits if len(descs[i]["key"]) == best}
            if len(top) > 1:
                amb += 1
                continue
            hit = next(descs[i] for i in fits if len(descs[i]["key"]) == best)
            how = "contained"
    if hit:
        named[key] = (hit["name"], hit["size"], hit["nf"], how)

cov = sum(len(groups[k]) for k in named)
print("identified: %d of %d structs (%.0f%%), %d of %d fields (%.0f%%)"
      % (len(named), len(groups), 100 * len(named) / len(groups),
         cov, len(rows), 100 * cov / len(rows)))
print("ambiguous (several descriptors equally contained): %d" % amb)
ns = collections.Counter(v[0].split(".")[0] for v in named.values())
print("by namespace: %s" % dict(ns.most_common()))

with open(OUT, "w", encoding="utf-8", newline="\n") as f:
    f.write("library\tstructId\tname\truntimeSize\townFields\ttableFields\tmatch\n")
    for k in sorted(groups):
        n = named.get(k)
        if n:
            f.write("%d\t%d\t%s\t%d\t%d\t%d\t%s\n"
                    % (k[0], k[1], n[0], n[1], n[2], len(groups[k]), n[3]))
        else:
            f.write("%d\t%d\t\t0\t0\t%d\t\n" % (k[0], k[1], len(groups[k])))
print("wrote %s" % OUT)
