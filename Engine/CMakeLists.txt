# ── Onyx engine library ───────────────────────────────────────────────────
#
# Reusable, game-agnostic SDK. Compiles WITHOUT any file under Apps/ — the
# boundary contract of the M2 split. Apps consume it via Onyx::Onyx and
# inherit its PUBLIC include root (Source/), so existing `#include "core/..."`
# / "ui/..." / "window/..." lines keep resolving.

file(GLOB_RECURSE ONYX_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

if(APPLE)
    file(GLOB_RECURSE ONYX_OBJC CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.m"
    )
    list(APPEND ONYX_SOURCES ${ONYX_OBJC})
endif()

add_library(Onyx STATIC ${ONYX_SOURCES})
add_library(Onyx::Onyx ALIAS Onyx)

target_include_directories(Onyx PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_features(Onyx PUBLIC cxx_std_20)

target_link_libraries(Onyx PUBLIC
    imgui_lib
    implot_lib
    imgui_color_text_edit
    glfw
    glad
    glm::glm
    lz4_lib
    miniaudio
    ffmpeg_lib
)

# MSVC source charset: UCN escapes ( etc. for SF Symbols / codicons) need
# /utf-8 so MSVC reads source as UTF-8 and emits UTF-8 narrow string literals.
# Without it cl.exe falls back to CP-1252 and warns C4566 on each codicon.
if(MSVC)
    target_compile_options(Onyx PUBLIC /utf-8)
endif()

# Expose the native GLFW API (glfwGetCocoaWindow, glfwGetWin32Window, etc.)
# and per-OS macros. PUBLIC so the app's translation units see them too.
target_compile_definitions(Onyx PUBLIC
    $<$<PLATFORM_ID:Windows>:GLFW_EXPOSE_NATIVE_WIN32>
    $<$<PLATFORM_ID:Darwin>:GLFW_EXPOSE_NATIVE_COCOA>
    $<$<PLATFORM_ID:Windows>:GOWTOOL_OS_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:GOWTOOL_OS_MACOS>
    $<$<PLATFORM_ID:Linux>:GOWTOOL_OS_LINUX>
    $<$<PLATFORM_ID:Windows>:NOMINMAX>   # stop minwindef.h defining min/max macros
)

# Platform link libraries.
if(WIN32)
    target_link_libraries(Onyx PUBLIC
        opengl32 comdlg32 dwmapi
    )
elseif(APPLE)
    target_link_libraries(Onyx PUBLIC
        "-framework OpenGL"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
    )
else()
    target_link_libraries(Onyx PUBLIC
        GL dl pthread m
    )
endif()
