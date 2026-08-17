// Regression: handlers self-register during static init, but their GetId()
// reads app-owned TypeId handles that are still 0 until the app seeds the
// TypeCatalog inside main(). TypeRegistry used to index eagerly at
// registration time, so every handler landed under id 0 and each one
// overwrote the last -- Resolve(realId) then returned nullptr and the UI
// reported "No viewer found for TypeId=N".

#include <doctest/doctest.h>
#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Types/TypeCatalog.h>

#include <memory>
#include <string>

namespace {

// Stand-ins for Onyx::GameTypes::X -- zero-initialised, filled in only once
// the catalog is seeded.
Onyx::Types::TypeId g_alphaId;
Onyx::Types::TypeId g_betaId;

struct AlphaHandler : Onyx::Types::ITypeHandler {
    Onyx::Types::TypeId GetId() const override { return g_alphaId; }
    const char* GetName() const override { return "AlphaHandler"; }
};

struct BetaHandler : Onyx::Types::ITypeHandler {
    Onyx::Types::TypeId GetId() const override { return g_betaId; }
    const char* GetName() const override { return "BetaHandler"; }
};

Onyx::Types::TypeId SeedType(const char* key, const char* label) {
    Onyx::Types::TypeInfo info;
    info.key   = key;
    info.label = label;
    return Onyx::Types::TypeCatalog::Get().Register(info);
}

} // namespace

TEST_CASE("[TypeRegistry] resolves handlers registered before the catalog is seeded") {
    auto& reg = Onyx::Types::TypeRegistry::Get();

    // --- static-init phase: ids are still the Unknown sentinel -------------
    REQUIRE(g_alphaId.value == 0);
    REQUIRE(g_betaId.value == 0);

    reg.RegisterByTypeId(std::make_unique<AlphaHandler>());
    reg.RegisterByTypeId(std::make_unique<BetaHandler>());

    // --- main() phase: app seeds the catalog, handles become valid ---------
    g_alphaId = SeedType("TEST_ALPHA_TYPE", "Alpha");
    g_betaId  = SeedType("TEST_BETA_TYPE",  "Beta");

    REQUIRE(g_alphaId.value != 0);
    REQUIRE(g_betaId.value  != 0);
    REQUIRE(g_alphaId.value != g_betaId.value);

    // --- lookup: both must resolve, and to the right handler ---------------
    auto* alpha = reg.Resolve(g_alphaId);
    auto* beta  = reg.Resolve(g_betaId);

    REQUIRE(alpha != nullptr);
    REQUIRE(beta  != nullptr);

    // The eager-index bug collapsed both into m_idMap[0], so the second
    // registration won and the first became unreachable.
    CHECK(std::string(alpha->GetName()) == "AlphaHandler");
    CHECK(std::string(beta->GetName())  == "BetaHandler");

    // The Unknown sentinel must never map to a real handler.
    CHECK(reg.Resolve(Onyx::Types::TypeId{0}) == nullptr);
}

TEST_CASE("[TypeRegistry] unregistered TypeIds still resolve to nullptr") {
    auto& reg = Onyx::Types::TypeRegistry::Get();
    CHECK(reg.Resolve(Onyx::Types::TypeId{0xDEADBEEF}) == nullptr);
}
