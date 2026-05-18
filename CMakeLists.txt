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
    # Hash obtido do erro de cmake na primeira execução (BtbN GPL shared 7.1 win64)
    FetchContent_Declare(
        ffmpeg_prebuilt
        URL      "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip"
        URL_HASH SHA256=4b40627b89007b0c47338ec4b4da15af03625d84a47e2651cc326859b783eee1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/ffmpeg
    )
    FetchContent_MakeAvailable(ffmpeg_prebuilt)

    # O ZIP do BtbN extrai um subdiretório com o nome do release.
    # Usamos GLOB para encontrá-lo automaticamente sem hardcode.
    file(GLOB FFMPEG_SUBDIRS "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/ffmpeg-*")
    if(FFMPEG_SUBDIRS)
        list(GET FFMPEG_SUBDIRS 0 FFMPEG_ROOT)
    else()
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/third_party/ffmpeg")
    endif()

    target_include_directories(ffmpeg_lib INTERFACE "${FFMPEG_ROOT}/include")
    target_link_libraries(ffmpeg_lib INTERFACE
        "${FFMPEG_ROOT}/lib/avformat.lib"
        "${FFMPEG_ROOT}/lib/avcodec.lib"
        "${FFMPEG_ROOT}/lib/avutil.lib"
        "${FFMPEG_ROOT}/lib/swscale.lib"
        "${FFMPEG_ROOT}/lib/swresample.lib"
    )

    # Armazena o caminho das DLLs — o add_custom_command vem depois
    # do add_executable, quando o target GoWToolkit já existe.
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

# ── gowtoolkit_parser_min ────────────────────────────────────────────────
#
# Parser-only static library containing just the bits of `src/core/` needed
# to run `ProfileGOW2::ParseWad` and `ProfileGOWR::ParseWad` end-to-end —
# headers, file-source abstractions, schema, type registry, profile cores.
# Excludes:
#   - `core/loaders/*`         — UI-coupled loader implementations
#   - `core/types/handlers/*`  — viewer-coupled type handlers
#   - `core/parsers/*`         — payload sub-parsers (not needed for WAD walk)
# This keeps the test binary lean and free of the UI dependency graph.
# Callers that exercise the live parsers (the GUI app, the test harness)
# link this target. The GUI additionally compiles the excluded sources for
# their UI/viewer integration; tests provide a small stub for any symbols
# the profiles reference into those layers (see `tests/test_stubs.cpp`).
set(PARSER_MIN_SOURCES
    src/core/Logger.cpp
    src/core/Metrics.cpp
    src/core/WadAssetName.cpp
    src/core/profiles/gow2/ProfileGOW2.cpp
    src/core/profiles/gow2/formats/MDL.cpp
    src/core/profiles/gowr/ProfileGOWR.cpp
    src/core/profiles/gowr/WadNodeBuilder.cpp
    src/core/schema/NodeInstance.cpp
    src/core/schema/StructDef.cpp
    src/core/types/TypeRegistry.cpp
    src/core/vfs/IsoFile.cpp
    src/core/vfs/IsoFileSystem.cpp
    src/core/vfs/OsFile.cpp
)

add_library(gowtoolkit_parser_min STATIC ${PARSER_MIN_SOURCES})
target_include_directories(gowtoolkit_parser_min PUBLIC src)
target_link_libraries(gowtoolkit_parser_min PUBLIC lz4_lib)
target_compile_features(gowtoolkit_parser_min PUBLIC cxx_std_20)

# ── GoWToolkit ────────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES src/*.cpp)
# Strip parser-min sources from the GLOB so the main executable links them
# from the static library instead of compiling duplicates.
foreach(_psrc IN LISTS PARSER_MIN_SOURCES)
    list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/${_psrc}")
endforeach()

if(APPLE)
    file(GLOB_RECURSE OBJC_SOURCES src/*.mm src/*.m)
    list(APPEND SOURCES ${OBJC_SOURCES})

    # Always create executable first
    add_executable(GoWToolkit ${SOURCES})

    # ── macOS .app bundle (ALL build types) ──────────────────────────────
    # Without a bundle + Info.plist the OS treats the process as a plain CLI
    # tool: no Dock entry, titlebar shows the terminal process name, and the
    # app cannot receive Cmd+Tab focus.  Enabling the bundle in Debug too
    # costs nothing (CLion/Xcode run the .app the same way) and fixes all
    # three symptoms for free.
    set_target_properties(GoWToolkit PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gowtoolkit.app"
        MACOSX_BUNDLE_BUNDLE_NAME "GoWToolkit"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/dist/macos/Info.plist.in"
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")

        # ── Compile .icon → Assets.car via actool ────────────────────────
        set(ICON_SOURCE "${CMAKE_SOURCE_DIR}/dist/macos/GoWToolkit.icon")
        set(ASSETS_CAR  "${CMAKE_CURRENT_BINARY_DIR}/Assets.car")

        add_custom_command(
            OUTPUT "${ASSETS_CAR}"
            COMMAND xcrun actool
                "${ICON_SOURCE}"
                --compile "${CMAKE_CURRENT_BINARY_DIR}"
                --app-icon "GoWToolkit"
                --platform macosx
                --minimum-deployment-target 13.0
                --output-partial-info-plist /dev/null
            DEPENDS "${ICON_SOURCE}/icon.json"
            COMMENT "Compiling GoWToolkit.icon → Assets.car (Liquid Glass)"
        )
        add_custom_target(CompileIcon ALL DEPENDS "${ASSETS_CAR}")
        add_dependencies(GoWToolkit CompileIcon)

        # ── Copy Assets.car into bundle Resources ────────────────────────
        add_custom_command(TARGET GoWToolkit POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ASSETS_CAR}"
                "$<TARGET_BUNDLE_CONTENT_DIR:GoWToolkit>/Resources/Assets.car"
            COMMENT "Copying Assets.car into bundle"
        )
    endif()

else()
    # ── Non-Apple: plain executable ──────────────────────────────────────
    add_executable(GoWToolkit ${SOURCES})
endif()

target_include_directories(GoWToolkit PRIVATE src third_party/bcdec)

# Expõe a API nativa do GLFW (glfwGetCocoaWindow, glfwGetWin32Window, etc.)
target_compile_definitions(GoWToolkit PRIVATE
    $<$<PLATFORM_ID:Windows>:GLFW_EXPOSE_NATIVE_WIN32>
    $<$<PLATFORM_ID:Darwin>:GLFW_EXPOSE_NATIVE_COCOA>
    $<$<PLATFORM_ID:Windows>:GOWTOOL_OS_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:GOWTOOL_OS_MACOS>
    $<$<PLATFORM_ID:Linux>:GOWTOOL_OS_LINUX>
    $<$<PLATFORM_ID:Windows>:NOMINMAX>   # Evita que minwindef.h defina min/max como macros
)

# Platform libs
if(WIN32)
    target_link_libraries(GoWToolkit PRIVATE
        gowtoolkit_parser_min
        imgui_lib implot_lib glfw glad glm::glm lz4_lib miniaudio ffmpeg_lib
        opengl32 comdlg32 dwmapi
    )
    set_target_properties(GoWToolkit PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
elseif(APPLE)
    target_link_libraries(GoWToolkit PRIVATE
        gowtoolkit_parser_min
        imgui_lib implot_lib glfw glad glm::glm lz4_lib miniaudio ffmpeg_lib
        "-framework OpenGL"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
    )
else()
    target_link_libraries(GoWToolkit PRIVATE
        gowtoolkit_parser_min
        imgui_lib implot_lib glfw glad lz4_lib miniaudio ffmpeg_lib GL dl pthread m
    )
endif()

# ── Copy Fonts ───────────────────────────────────────────────────────────
# macOS always uses a bundle now (Debug and Release), so always copy into
# Resources.  Non-Apple platforms copy next to the executable as before.
if(APPLE)
    add_custom_command(TARGET GoWToolkit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/third_party/fonts"
            "$<TARGET_BUNDLE_CONTENT_DIR:GoWToolkit>/Resources/third_party/fonts"
        COMMENT "Copying fonts into bundle Resources"
    )
else()
    add_custom_command(TARGET GoWToolkit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/third_party/fonts"
            "$<TARGET_FILE_DIR:GoWToolkit>/third_party/fonts"
        COMMENT "Copying fonts to output directory"
    )
endif()

# ── Copia DLLs do FFmpeg (Windows only) ───────────────────────────────────
if(WIN32 AND FFMPEG_DLLS)
    add_custom_command(TARGET GoWToolkit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FFMPEG_DLLS}
            "$<TARGET_FILE_DIR:GoWToolkit>"
        COMMENT "Copying FFmpeg DLLs to output directory"
    )
endif()

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
endif()