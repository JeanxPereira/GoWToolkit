cmake_minimum_required(VERSION 3.20)
project(GoWToolkit CXX C)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
include(CTest)

# ── ImGui (docking branch) ─────────────────────────────────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/third_party/imgui
    GIT_SHALLOW    TRUE        # só baixa o commit atual, sem histórico completo
    UPDATE_DISCONNECTED TRUE   # nunca tenta git fetch depois do primeiro clone
)
FetchContent_MakeAvailable(imgui)

# ── GLFW ───────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.9
    GIT_SHALLOW    TRUE
    UPDATE_DISCONNECTED TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ── GLM (header-only math) ────────────────────────────────────────────────
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
    UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(glm)

# ── lz4 ───────────────────────────────────────────────────────────────────
FetchContent_Declare(
    lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    GIT_TAG        v1.9.4
    GIT_SHALLOW    TRUE
    UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(lz4)

add_library(lz4_lib STATIC
    ${lz4_SOURCE_DIR}/lib/lz4.c
    ${lz4_SOURCE_DIR}/lib/lz4frame.c
    ${lz4_SOURCE_DIR}/lib/lz4hc.c
    ${lz4_SOURCE_DIR}/lib/xxhash.c
)
target_include_directories(lz4_lib PUBLIC ${lz4_SOURCE_DIR}/lib)

# ── glad (manual) ─────────────────────────────────────────────────────────
add_library(glad STATIC
    third_party/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    third_party/glad/include
)

# ── miniaudio (single-header audio) ──────────────────────────────────────
add_library(miniaudio STATIC
    third_party/miniaudio/miniaudio.c
)
target_include_directories(miniaudio PUBLIC third_party/miniaudio)

# ── ImGui lib ──────────────────────────────────────────────────────────────
add_library(imgui_lib STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    third_party/imgui
    third_party/imgui/backends
)
target_link_libraries(imgui_lib PUBLIC glfw glad)

# ── ImPlot (2D plotting for ImGui) ────────────────────────────────────────
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        master
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/third_party/implot
    GIT_SHALLOW    TRUE
    UPDATE_DISCONNECTED TRUE
)
FetchContent_Populate(implot)

add_library(implot_lib STATIC
    third_party/implot/implot.cpp
    third_party/implot/implot_items.cpp
)
target_include_directories(implot_lib PUBLIC third_party/implot)
target_link_libraries(implot_lib PUBLIC imgui_lib)

# ── ImGuiColorTextEdit (Goossens fork — submodule) ───────────────────────
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/imgui_color_text_edit/TextEditor.cpp")
    message(FATAL_ERROR
        "third_party/imgui_color_text_edit is empty. "
        "Run: git submodule update --init --recursive")
endif()
add_library(imgui_color_text_edit STATIC
    third_party/imgui_color_text_edit/TextEditor.cpp
)
target_include_directories(imgui_color_text_edit PUBLIC
    third_party/imgui_color_text_edit
)
target_link_libraries(imgui_color_text_edit PUBLIC imgui_lib)
target_compile_features(imgui_color_text_edit PUBLIC cxx_std_17)

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

# ── FFmpeg ────────────────────────────────────────────────────────────────
#
#   Windows : baixa prebuilt (BtbN GPL shared) via FetchContent
#   Linux   : usa pkg-config  (instale: sudo apt install libavformat-dev
#                               libavcodec-dev libswscale-dev libswresample-dev)
#   macOS   : usa pkg-config  (instale: brew install ffmpeg)
#
add_library(ffmpeg_lib INTERFACE)

if(WIN32)
    # ── Prebuilt Windows (BtbN – GPL shared, x64) ────────────────────
    # BtbN "latest" tag is a rolling pointer — the asset SHA256 changes
    # whenever upstream rebuilds. Pinning URL_HASH guarantees CI breakage
    # the moment that happens, so we omit it and rely on TLS for transport
    # integrity. To pin a specific build, swap the URL to a dated tag
    # (autobuild-YYYY-MM-DD-HH-MM) and reinstate URL_HASH.
    # Extract into default `_deps/ffmpeg_prebuilt-src` so the
    # `actions/cache` step (path: build/_deps) actually captures it.
    # Earlier versions used a `SOURCE_DIR` override into the source tree,
    # which broke under cache restore: the FetchContent subbuild stamp
    # was cached and reported "populated", but the source tree extraction
    # was not, so headers vanished and Windows compiles failed with
    # `Cannot open include file: 'libavformat/avformat.h'`.
    FetchContent_Declare(
        ffmpeg_prebuilt
        URL      "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(ffmpeg_prebuilt)

    # The ZIP extracts as `<src>/ffmpeg-n7.1-.../{include,lib,bin}`.
    # Locate that nested dir.
    file(GLOB FFMPEG_SUBDIRS "${ffmpeg_prebuilt_SOURCE_DIR}/ffmpeg-*")
    if(FFMPEG_SUBDIRS)
        list(GET FFMPEG_SUBDIRS 0 FFMPEG_ROOT)
    else()
        set(FFMPEG_ROOT "${ffmpeg_prebuilt_SOURCE_DIR}")
    endif()

    if(NOT EXISTS "${FFMPEG_ROOT}/include/libavformat/avformat.h")
        message(FATAL_ERROR
            "FFmpeg headers not found at ${FFMPEG_ROOT}/include. "
            "FetchContent extraction likely failed or upstream zip layout "
            "changed. Delete build/_deps/ffmpeg_prebuilt-* and reconfigure.")
    endif()

    target_include_directories(ffmpeg_lib INTERFACE "${FFMPEG_ROOT}/include")
    target_link_libraries(ffmpeg_lib INTERFACE
        "${FFMPEG_ROOT}/lib/avformat.lib"
        "${FFMPEG_ROOT}/lib/avcodec.lib"
        "${FFMPEG_ROOT}/lib/avutil.lib"
        "${FFMPEG_ROOT}/lib/swscale.lib"
        "${FFMPEG_ROOT}/lib/swresample.lib"
    )

    # Path to DLLs — copy step lives after add_executable below.
    file(GLOB FFMPEG_DLLS "${FFMPEG_ROOT}/bin/*.dll")

else()
    # ── Linux / macOS via pkg-config ──────────────────────────────────
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED
        libavformat
        libavcodec
        libswscale
        libswresample
        libavutil
    )

    target_include_directories(ffmpeg_lib INTERFACE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(ffmpeg_lib      INTERFACE ${FFMPEG_LIBRARIES})
    target_compile_options(ffmpeg_lib     INTERFACE ${FFMPEG_CFLAGS_OTHER})
    target_link_directories(ffmpeg_lib    INTERFACE ${FFMPEG_LIBRARY_DIRS})
endif()
# -- Targets -----------------------------------------------------------------
#
# The single GoWToolkit target was split (M2) into:
#   - Onyx       : reusable, game-agnostic engine static library
#   - GoWToolkit : the executable (first consumer of Onyx)
# Both live in their own CMakeLists; the third-party blocks above are shared.
add_subdirectory(Engine)
add_subdirectory(Apps/GoWToolkit)

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
endif()
