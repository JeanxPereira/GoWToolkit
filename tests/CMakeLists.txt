# ── GoWToolkit unit tests (doctest) ───────────────────────────────────────
#
# Built when -DGOWTOOLKIT_BUILD_TESTS=ON (default).  The runner is wired into
# CTest by the root CMakeLists via add_test(NAME unit ...).

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(gowtoolkit_tests ${TEST_SOURCES})

target_include_directories(gowtoolkit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Pass the path to the fixtures directory so tests don't depend on the
# current working directory at execution time.
target_compile_definitions(gowtoolkit_tests PRIVATE
    GOWTOOLKIT_TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

target_link_libraries(gowtoolkit_tests PRIVATE
    doctest::doctest
    nlohmann_json::nlohmann_json
    gowtoolkit_parser_min
)

set_target_properties(gowtoolkit_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
