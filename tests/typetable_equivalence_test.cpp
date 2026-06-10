#include <doctest/doctest.h>
#include "core/types/GameTypeTable.h"
#include "core/types/TypeId.h"
#include "core/domain/MediaKind.h"
#include <cstring>

using namespace Onyx;

TEST_CASE("GameTypeTable reproduces legacy KindOf and TypeIdName") {
    // Row count matches the enum (excluding COUNT).
    CHECK((int)(sizeof(kGameTypeTable)/sizeof(kGameTypeTable[0])) == (int)TypeId::COUNT);

    for (const auto& row : kGameTypeTable) {
        TypeId legacy = static_cast<TypeId>(row.legacyValue);
        // legacyValue is exactly the enum numeric value
        CHECK((uint32_t)legacy == row.legacyValue);
        // media metadata matches the old constexpr switch
        CHECK(KindOf(legacy) == row.media);
        // label metadata matches the old TypeIdName switch
        CHECK(std::strcmp(TypeIdName(legacy), row.label) == 0);
    }
}
