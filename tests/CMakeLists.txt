# ── GoWToolkit unit tests (doctest) ───────────────────────────────────────
#
# Built when -DGOWTOOLKIT_BUILD_TESTS=ON (default).  The runner is wired into
# CTest by the root CMakeLists via add_test(NAME unit ...).

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(gowtoolkit_tests ${TEST_SOURCES})

target_include_directories(gowtoolkit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(gowtoolkit_tests PRIVATE
    doctest::doctest
)

set_target_properties(gowtoolkit_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
