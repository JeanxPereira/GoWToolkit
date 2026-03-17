cmake_minimum_required(VERSION 3.20)
project(GoWToolkit CXX C)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# ── ImGui (docking branch) ─────────────────────────────────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/third_party/imgui
)
FetchContent_MakeAvailable(imgui)

# ── GLFW ───────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.9
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
)
FetchContent_MakeAvailable(glm)

# ── lz4 ───────────────────────────────────────────────────────────────────
FetchContent_Declare(
    lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    GIT_TAG        v1.9.4
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

# ── GoWToolkit ────────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES src/*.cpp)
if(APPLE)
    file(GLOB_RECURSE OBJC_SOURCES src/*.mm src/*.m)
    list(APPEND SOURCES ${OBJC_SOURCES})
endif()
add_executable(GoWToolkit ${SOURCES})

target_include_directories(GoWToolkit PRIVATE src)

# Expõe a API nativa do GLFW (glfwGetCocoaWindow, glfwGetWin32Window, etc.)
target_compile_definitions(GoWToolkit PRIVATE
    $<$<PLATFORM_ID:Windows>:GLFW_EXPOSE_NATIVE_WIN32>
    $<$<PLATFORM_ID:Darwin>:GLFW_EXPOSE_NATIVE_COCOA>
    $<$<PLATFORM_ID:Windows>:GOWTOOL_OS_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:GOWTOOL_OS_MACOS>
    $<$<PLATFORM_ID:Linux>:GOWTOOL_OS_LINUX>
)

# Platform libs
if(WIN32)
    target_link_libraries(GoWToolkit PRIVATE
        imgui_lib glfw glad glm::glm lz4_lib miniaudio
        opengl32 comdlg32 dwmapi
    )
    set_target_properties(GoWToolkit PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
elseif(APPLE)
    target_link_libraries(GoWToolkit PRIVATE
        imgui_lib glfw glad glm::glm lz4_lib miniaudio
        "-framework OpenGL"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
    )
else()
    target_link_libraries(GoWToolkit PRIVATE
        imgui_lib glfw glad lz4_lib miniaudio GL dl pthread m
    )
endif()

# ── Copia todas as fontes para o diretório de output ─────────────────────
add_custom_command(TARGET GoWToolkit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/third_party/fonts"
        "$<TARGET_FILE_DIR:GoWToolkit>/third_party/fonts"
    COMMENT "Copying fonts to output directory"
)
