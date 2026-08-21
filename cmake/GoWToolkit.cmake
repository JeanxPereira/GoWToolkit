# ── GoWToolkit application ─────────────────────────────────────────────────
#
# First consumer of the Onyx engine. Owns the per-executable concerns: the
# macOS .app bundle, the FFmpeg DLL copy, the fonts copy, WIN32_EXECUTABLE,
# and the app icon. Links Onyx::Onyx and inherits its PUBLIC include root.

file(GLOB_RECURSE GOWTOOLKIT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

if(APPLE)
    file(GLOB_RECURSE GOWTOOLKIT_OBJC CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.m"
    )
    list(APPEND GOWTOOLKIT_SOURCES ${GOWTOOLKIT_OBJC})

    add_executable(GoWToolkit ${GOWTOOLKIT_SOURCES})

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

    # ── Compile .icon → Assets.car via actool ────────────────────────────
    # actool ships with full Xcode, not Command Line Tools. If the active
    # developer dir (`xcode-select -p`) points at CLT, `xcrun actool` fails
    # even when Xcode.app is installed. Find Xcode.app explicitly and use
    # its actool directly to avoid that dependency.
    set(_actool_path "")
    execute_process(
        COMMAND xcrun --find actool
        RESULT_VARIABLE _xcrun_result
        OUTPUT_VARIABLE _xcrun_actool
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_xcrun_result EQUAL 0 AND EXISTS "${_xcrun_actool}")
        set(_actool_path "${_xcrun_actool}")
    else()
        foreach(_xcode_app
            "/Applications/Xcode.app"
            "/Applications/Xcode-beta.app")
            set(_candidate "${_xcode_app}/Contents/Developer/usr/bin/actool")
            if(EXISTS "${_candidate}")
                set(_actool_path "${_candidate}")
                break()
            endif()
        endforeach()
    endif()

    if(_actool_path)
        message(STATUS "Using actool: ${_actool_path}")
        set(ICON_SOURCE "${CMAKE_SOURCE_DIR}/dist/macos/GoWToolkit.icon")
        set(ASSETS_CAR  "${CMAKE_CURRENT_BINARY_DIR}/Assets.car")
        set(ASSETS_STAMP "${CMAKE_CURRENT_BINARY_DIR}/Assets.stamp")

        # actool may exit 0 without producing Assets.car if it does not
        # understand the .icon (Liquid Glass) format — this happens with
        # Xcode 15.x. Always emit a stamp file so the custom target stays
        # valid; the POST_BUILD copy below guards on Assets.car existing.
        add_custom_command(
            OUTPUT "${ASSETS_STAMP}"
            COMMAND "${_actool_path}"
                "${ICON_SOURCE}"
                --compile "${CMAKE_CURRENT_BINARY_DIR}"
                --app-icon "GoWToolkit"
                --platform macosx
                --minimum-deployment-target 13.0
                --output-partial-info-plist /dev/null
                || true
            COMMAND ${CMAKE_COMMAND} -E touch "${ASSETS_STAMP}"
            DEPENDS "${ICON_SOURCE}/icon.json"
            COMMENT "Compiling GoWToolkit.icon → Assets.car (Liquid Glass)"
        )
        add_custom_target(CompileIcon ALL DEPENDS "${ASSETS_STAMP}")
        add_dependencies(GoWToolkit CompileIcon)

        # ── Copy Assets.car into bundle Resources (only if produced) ─────
        # Guarded with `test -f` because Xcode 15.x actool silently no-ops
        # on .icon and leaves Assets.car absent.
        add_custom_command(TARGET GoWToolkit POST_BUILD
            COMMAND bash -c
                "if [ -f '${ASSETS_CAR}' ]; then \
                    '${CMAKE_COMMAND}' -E copy_if_different \
                        '${ASSETS_CAR}' \
                        '$<TARGET_BUNDLE_CONTENT_DIR:GoWToolkit>/Resources/Assets.car'; \
                 else \
                    echo 'Skipping Assets.car copy: file not produced (Xcode 15.x cannot compile .icon)'; \
                 fi"
            COMMENT "Copying Assets.car into bundle (if present)"
            VERBATIM
        )
    else()
        message(WARNING
            "actool not found (requires full Xcode). App will use a generic "
            "Dock icon. Install Xcode and run: "
            "sudo xcode-select -s /Applications/Xcode.app")
    endif()

else()
    # ── Non-Apple: plain executable ──────────────────────────────────────
    add_executable(GoWToolkit ${GOWTOOLKIT_SOURCES})
endif()

target_include_directories(GoWToolkit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/third_party/bcdec
    # stb_image_write: PNG output for the headless `render` command.
    ${CMAKE_SOURCE_DIR}/third_party/stb
)

# Onyx::Onyx aggregates Core + Render + Shell only. The CLI additionally
# needs CmdRender (Onyx::CliRender) and the glTF exporter (Onyx::Exchange),
# which the aggregate deliberately leaves out so a GUI-only consumer does
# not pay for them.
target_link_libraries(GoWToolkit PRIVATE Onyx::Onyx Onyx::CliRender Onyx::Exchange)

if(WIN32)
    set_target_properties(GoWToolkit PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
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

# ── Copy FFmpeg DLLs (Windows only) ───────────────────────────────────────
# ONYX_FFMPEG_DLLS is a CACHE INTERNAL var exported by OnyxSDK (consumed via
# FetchContent in the root CMakeLists). Using ONYX_FFMPEG_DLLS instead of the
# former FFMPEG_DLLS which lived in root parent scope.
if(WIN32 AND ONYX_FFMPEG_DLLS)
    add_custom_command(TARGET GoWToolkit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ONYX_FFMPEG_DLLS}
            "$<TARGET_FILE_DIR:GoWToolkit>"
        COMMENT "Copying FFmpeg DLLs to output directory"
    )
endif()

# -- Copy dxcompiler.dll for DXIL disassembly (Windows, optional) ----------
# The shader viewer decodes the DXBC container natively but delegates DXIL
# disassembly to dxcompiler.dll, which it loads by name at runtime. Copying
# it next to the executable makes that work out of the box; when it is not
# found the viewer degrades to the structured view and says so, so this is a
# convenience rather than a requirement.
if(WIN32)
    file(GLOB _dxc_candidates
        "$ENV{WindowsSdkDir}bin/*/x64/dxcompiler.dll"
        "C:/Program Files (x86)/Windows Kits/10/bin/*/x64/dxcompiler.dll")
    if(_dxc_candidates)
        list(SORT _dxc_candidates)
        list(GET _dxc_candidates -1 DXCOMPILER_DLL)
        message(STATUS "DXIL disassembly: using ${DXCOMPILER_DLL}")
        add_custom_command(TARGET GoWToolkit POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DXCOMPILER_DLL}"
                "$<TARGET_FILE_DIR:GoWToolkit>"
            COMMENT "Copying dxcompiler.dll to output directory"
        )
    else()
        message(STATUS "DXIL disassembly: dxcompiler.dll not found; the shader "
                       "viewer will show the container view only")
    endif()
endif()
