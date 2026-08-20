#include <doctest/doctest.h>
#include "core/types/GameTypeTable.h"
#include <Onyx/Types/TypeId.h>
#include "core/types/GameTypes.h"
#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Domain/MediaKind.h>
#include <cstring>

using namespace Onyx;
using namespace Onyx::Domain;
using namespace Onyx::Types;

TEST_CASE("GameTypeTable seeds the catalog with stable values, media and labels") {
    GameTypes::RegisterGameTypes();

    for (const auto& row : kGameTypeTable) {
        Types::TypeId id{row.legacyValue};
        // The registered handle's value must equal its legacy enum position.
        CHECK(id.value == row.legacyValue);
        // media metadata matches the table (which mirrors the old KindOf switch)
        CHECK(TypeCatalog::Get().Media(id) == row.media);
        // label metadata matches the table (which mirrors the old TypeIdName)
        CHECK(std::strcmp(TypeCatalog::Get().Label(id), row.label) == 0);
    }
}
