// Regression: the global selection (Onyx::Api::GetSelected / GetSelectedWad)
// holds raw pointers into AssetDatabase::wads. Closing a WAD erases that
// vector element and frees the AssetEntry storage, but nothing cleared the
// selection -- so Inspector::Draw() dereferenced freed memory on the next
// frame and the app crashed right after "Close WAD".

#include <doctest/doctest.h>
#include <Onyx/Api/ToolkitApi.h>
#include <Onyx/Services/AssetDatabase.h>
#include <Onyx/Services/Events.h>

TEST_CASE("[Selection] closing a WAD drops the selection before it dangles") {
    Onyx::Services::AssetDatabase db;
    Onyx::Api::InitParams params;
    params.db = &db;
    Onyx::Api::Init(params);

    AssetContainer wad;
    AssetEntry     entry;

    Onyx::Api::SetSelected(&entry, &wad);
    REQUIRE(Onyx::Api::GetSelected()    == &entry);
    REQUIRE(Onyx::Api::GetSelectedWad() == &wad);

    // CloseWad posts this before erasing, so the selection must already be
    // gone by the time the storage dies.
    EventWadClosed::post(size_t{0});

    CHECK(Onyx::Api::GetSelected()    == nullptr);
    CHECK(Onyx::Api::GetSelectedWad() == nullptr);
}

TEST_CASE("[Selection] closing everything drops the selection") {
    Onyx::Services::AssetDatabase db;
    Onyx::Api::InitParams params;
    params.db = &db;
    Onyx::Api::Init(params);

    AssetContainer wad;
    AssetEntry     entry;

    Onyx::Api::SetSelected(&entry, &wad);
    REQUIRE(Onyx::Api::GetSelected() == &entry);

    db.CloseAll();

    CHECK(Onyx::Api::GetSelected()    == nullptr);
    CHECK(Onyx::Api::GetSelectedWad() == nullptr);
}
