cmake_minimum_required(VERSION 3.20)
project(GoWToolkit CXX C)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
include(CTest)

# ── OnyxSDK (engine + deps) ───────────────────────────────────────────────
# Provides Onyx::Onyx + all transitive deps (imgui_lib, glfw, glm::glm,
# lz4_lib, glad, miniaudio, implot_lib, imgui_color_text_edit, ffmpeg_lib)
# and CACHE vars ONYX_FFMPEG_DLLS / ONYX_FONTS_DIR.
#
# Local-dev override (iterate against local OnyxSDK without re-tagging):
#   cmake -DFETCHCONTENT_SOURCE_DIR_ONYXSDK=E:/CodingProjects/OnyxSDK ...
FetchContent_Declare(OnyxSDK
    GIT_REPOSITORY https://github.com/JeanxPereira/OnyxSDK.git
    # v1.1.0's tag does not exist yet -- PR #1 is open and the tag is cut at
    # merge. Pinned to the branch head until then; swap this for v1.1.0 once
    # the tag exists.
    GIT_TAG        5d5aeb9ee00ffb2ad382d91450a83baa2147155c)
set(ONYX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ONYX_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(OnyxSDK)
# Onyx::Onyx + imgui_lib/glfw/glm::glm/lz4_lib/miniaudio/ffmpeg_lib +
# ONYX_FFMPEG_DLLS / ONYX_FONTS_DIR are now available.

# ── bcdec (app-only, image-decoder single-header — NOT part of Onyx) ────
# Referenced from cmake/GoWToolkit.cmake via
#   target_include_directories(... third_party/bcdec)
# No CMake target needed; the include dir is set app-side.

# ── doctest (unit-test framework, dev-only) ──────────────────────────────
option(GOWTOOLKIT_BUILD_TESTS "Build the GoWToolkit unit test suite" ON)

if(GOWTOOLKIT_BUILD_TESTS)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.4.11
        GIT_SHALLOW    TRUE
        UPDATE_DISCONNECTED TRUE
    )
    set(DOCTEST_WITH_TESTS    OFF CACHE BOOL "" FORCE)
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
    # doctest v2.4.11 ships a pre-CMake-3.5 cmake_minimum_required; bypass the
    # hard error that CMake 4.x raises for that. Remove once doctest publishes
    # a release with the floor bumped.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
    FetchContent_MakeAvailable(doctest)
    unset(CMAKE_POLICY_VERSION_MINIMUM CACHE)

    # ── nlohmann/json (header-only, golden-test snapshot I/O) ────────────
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
        UPDATE_DISCONNECTED TRUE
    )
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install    OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ── Targets -----------------------------------------------------------------
#
# Engine (Onyx) is now consumed from OnyxSDK via FetchContent above.
# GoWToolkit is the executable (first consumer of Onyx::Onyx); its target is
# defined in cmake/GoWToolkit.cmake and consumes Source/ at the repo root.
include(cmake/GoWToolkit.cmake)

# ── Tests ─────────────────────────────────────────────────────────────────
if(GOWTOOLKIT_BUILD_TESTS AND BUILD_TESTING)
    add_subdirectory(tests)
    add_test(NAME unit COMMAND $<TARGET_FILE:gowtoolkit_tests>)
    # Individual entries so `ctest -R Golden` filters cleanly. The full
    # suite is still reachable via `ctest -R unit` or no filter at all.
    add_test(NAME Golden_GOW2
             COMMAND $<TARGET_FILE:gowtoolkit_tests> "--test-case=*GOW2*")
    add_test(NAME Golden_GOWR
             COMMAND $<TARGET_FILE:gowtoolkit_tests> "--test-case=*GOWR*")
    add_test(NAME Metrics
             COMMAND $<TARGET_FILE:gowtoolkit_tests> "--test-case=*Metrics*")
    add_test(NAME Logger
             COMMAND $<TARGET_FILE:gowtoolkit_tests> "--test-case=*Logger*")
    add_test(NAME Threading
             COMMAND $<TARGET_FILE:gowtoolkit_tests> "--test-case=*Threading*")
    add_test(NAME ThemeContrast
             COMMAND $<TARGET_FILE:gowtoolkit_tests> "--test-case=*ThemeContrast*")
    add_test(NAME GowrClassify
             COMMAND $<TARGET_FILE:gowtoolkit_tests> "--test-case=*GowrClassify*")
endif()
