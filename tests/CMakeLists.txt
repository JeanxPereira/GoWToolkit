# ── GoWToolkit unit tests (doctest) ───────────────────────────────────────
#
# Built when -DGOWTOOLKIT_BUILD_TESTS=ON (default).  The runner is wired into
# CTest by the root CMakeLists via add_test(NAME unit ...).
#
# Links the Onyx engine library (which carries the generic services —
# ThemeManager, AssetVisibility, ViewerRegistry, schema, vfs, platform theme,
# etc.) and additionally compiles the small set of APP-core sources the golden
# / table tests exercise directly (the game profiles + parsers + GameTypes).
# `test_stubs.cpp` still provides the GOWR loader symbol the profiles reach
# into, so the UI-coupled loader/handler layers stay out of the test binary.

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# theme_stubs.cpp stubbed Platform::DetectSystemAppearance() back when the test
# binary did not compile the platform .cpp. Onyx now carries the real
# SystemTheme implementation, so the stub would be a duplicate symbol — drop it.
list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/theme_stubs.cpp")

# All three tests the port excluded are back in the target (Phase 6):
#   - selection_lifetime_test.cpp: rewritten against Workspace/NodePath, the
#     model that replaced the AssetDatabase pointers whose dangling it used
#     to guard.
#   - visibility_persistence_test.cpp: rewritten against SaveOverrides/
#     LoadOverrides, which persist by catalog key instead of numeric TypeId.
#   - logger_test.cpp: the GOW_LOG_*/LOG_INFO macros it uses are the
#     ONYX_LOG_*/ONYX_LOGF_* families under their current names; it was a
#     rename, not an orphan.

# ── App-core sources the golden / table tests link against ────────────────
# These live under Source/ (game-specific, not part of Onyx).
# Minimal closure to run Gow2Module::ParseContainer / GowrModule::Parse-
# Container end-to-end (via Onyx::Modules::Workspace, see golden_helpers.cpp)
# plus the WadNodeBuilder/GowrTaxonomy machinery GowrModule calls into.
#
# ProfileGOW2.cpp/ProfileGOWR.cpp (and ProfileGOW2's only consumer,
# formats/MDL.cpp) are gone as of Phase 2 Task 4 -- golden_helpers.cpp now
# opens fixtures through Gow2Module/GowrModule on a Workspace, and nothing
# else in this target referenced the profile classes (see task-4-report.md).
# GameTypes.cpp stays: several existing unit tests (mediakind_test.cpp,
# typetable_equivalence_test.cpp, viewer_registry_test.cpp, gowr_classify_
# test.cpp, and GowrTaxonomy.cpp itself) call Onyx::GameTypes::RegisterGame-
# Types()/reference its externs directly and are unrelated to Task 4's
# module-typeId convergence -- that legacy catalog is still a real, tested
# surface, just no longer the one Gow2Module/GowrModule stamp trees with.
set(APP_TEST_SOURCES
    "${CMAKE_SOURCE_DIR}/Source/core/types/GameTypes.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/types/WadDispatch.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/profiles/gowr/WadNodeBuilder.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/profiles/gowr/GowrTaxonomy.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/parsers/gowr/MgParser.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/parsers/gowr/MaterialParser.cpp"
    # GOWRDecodeTexture / GOW2TextureParser::Parse -- GowrModule::
    # DecodeTexturePair and Gow2Module::DecodeImage call these directly
    # (Phase 2 Tasks 2/3). Both compiled cleanly in isolation before Task 4
    # but their symbols were never actually LINKED into gowtoolkit_tests
    # until now (the target didn't link at all pre-Task-4) -- discovered as
    # LNK2019 unresolved externals once the goldens started exercising a
    # full link. Self-contained: no UI-coupled loader/handler dependency.
    "${CMAKE_SOURCE_DIR}/Source/core/parsers/gowr/TextureDecode.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/parsers/gow2/TextureParser.cpp"
    # TexPackIndex::FindTexture/GetFile -- GOWRDecodeTexture (above) calls
    # these directly; test_stubs.cpp only stubs the process-wide GetTexIndex()
    # accessor, not TexPackIndex's own methods. Same "compiled but never
    # linked before Task 4" discovery as the two texture parsers above.
    "${CMAKE_SOURCE_DIR}/Source/core/parsers/gowr/TexPackIndex.cpp"
    # GowrModule (Onyx v1.1 IGameModule port of ProfileGOWR, Phase 2 Task 2).
    # Exercised directly by golden_helpers.cpp's RunGoldenTest since Task 4.
    "${CMAKE_SOURCE_DIR}/Source/core/modules/GowrModule.cpp"
    # Gow2Module (Onyx v1.1 IGameModule port of ProfileGOW2, Phase 2 Task 3).
    # Same, exercised directly since Task 4.
    "${CMAKE_SOURCE_DIR}/Source/core/modules/Gow2Module.cpp"
    # Phase 3 material model. Gow2SceneBuild carries the one rule the port
    # changed rather than merely re-spelled (SelectMainLayer -- see its
    # definition), and TextureRoles carries the partial GOWR->Onyx role
    # mapping. gow2_material_test.cpp pins both; without these two the app
    # target is the only thing that compiles them and nothing asserts them.
    "${CMAKE_SOURCE_DIR}/Source/core/types/Gow2SceneBuild.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/types/TextureRoles.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/parsers/gow2/MaterialParser.cpp"
)

add_executable(gowtoolkit_tests ${TEST_SOURCES} ${APP_TEST_SOURCES})

target_include_directories(gowtoolkit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/third_party/bcdec
)
# Engine headers are inherited transitively via Onyx::Onyx PUBLIC interface
# (<Onyx/...> includes). Engine/Source is no longer in-tree (now in OnyxSDK).

# Pass the path to the fixtures directory so tests don't depend on the
# current working directory at execution time.
target_compile_definitions(gowtoolkit_tests PRIVATE
    GOWTOOLKIT_TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

target_link_libraries(gowtoolkit_tests PRIVATE
    doctest::doctest
    nlohmann_json::nlohmann_json
    Onyx::Onyx
)

set_target_properties(gowtoolkit_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(gowtoolkit_tests PRIVATE /utf-8)
endif()

# MinimalViewer_SelfTest has moved to OnyxSDK/Tests (MinimalViewer now lives
# in OnyxSDK/Examples). GoWToolkit no longer builds or tests it.
