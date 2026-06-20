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

# ── App-core sources the golden / table tests link against ────────────────
# These live under Source/ (game-specific, not part of Onyx).
# Minimal closure to run ProfileGOW2::ParseContainer / ProfileGOWR::Parse-
# Container end-to-end plus the type catalog the snapshots label entries with.
set(APP_TEST_SOURCES
    "${CMAKE_SOURCE_DIR}/Source/core/types/GameTypes.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/types/WadDispatch.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/profiles/gow2/ProfileGOW2.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/profiles/gow2/formats/MDL.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/profiles/gowr/ProfileGOWR.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/profiles/gowr/WadNodeBuilder.cpp"
    "${CMAKE_SOURCE_DIR}/Source/core/profiles/gowr/GowrTaxonomy.cpp"
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
