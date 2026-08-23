#pragma once

// Queries over the generated smschema tables.
//
// SmSchemaTable.h is machine-written and holds nothing but data plus the three
// lookups the generator emits. Everything a caller actually wants to ask --
// what a type code means, which structs carry a given field, whether the table
// survived its last regeneration intact -- lives here, hand-written, so that
// regenerating the tables never overwrites it.
//
// A struct is identified by the PAIR (library, id): the id is library-local
// and restarts per registered library, so an id alone names a different struct
// in each one. See the header comment in SmSchemaTable.h for the record
// layouts and for how the names were recovered.

#include "core/formats/gowr/SmSchemaTable.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Onyx::Gowr::SmSchema {

// Human-readable name for a field's type code. Never returns nullptr: codes
// with no SmType entry come back as "unknown(0x....)" rather than being
// silently rendered as something they are not.
const char* TypeName(uint16_t typeCode);

// The field at a given byte offset, or nullptr. Offsets are unique within a
// struct in every table generated so far, but this returns the first match
// rather than asserting it, because that is a property of the extracted data
// and not something the format guarantees.
const Field* FindFieldAt(uint16_t library, uint16_t id, uint16_t offset);

// Every struct registered under a type name. A name legitimately appears more
// than once -- each animation node library re-declares dctools.AnimNode -- so
// this returns all of them; FindStructByName() in the generated header returns
// only the first.
std::vector<const Struct*> FindStructsNamed(const char* name);

// Every struct carrying a field with this exact name. Useful in reverse: given
// a field seen in a file, find which types could have produced it.
std::vector<const Struct*> FindStructsWithField(const char* fieldName);

struct Stats {
    int structs = 0;
    int namedStructs = 0;
    int fields = 0;
    int libraries = 0;
    int distinctFieldNames = 0;
};

Stats GetStats();

// Self-check of the generated tables, for the test suite and for anyone who
// has just regenerated them against a patched game.
//
// These are invariants of the EXTRACTION, not of the file format: they hold
// because of how the generator groups and sorts records, so a change in the
// game that broke one would show up here as a failure to investigate rather
// than as silently wrong lookups. Appends a line per problem and returns true
// when the table is clean.
bool Validate(std::vector<std::string>& problems);

} // namespace Onyx::Gowr::SmSchema
