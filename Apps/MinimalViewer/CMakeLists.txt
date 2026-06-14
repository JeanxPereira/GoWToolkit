# -- MinimalViewer -- second Onyx consumer (boundary proof) ------------------
# Knows nothing about God of War. Links ONLY Onyx::Onyx. Reaches the SDK only
# through <Onyx/...> public headers (inherits Onyx's PUBLIC Include/ root).

file(GLOB MINIMALVIEWER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

add_executable(MinimalViewer ${MINIMALVIEWER_SOURCES})
target_include_directories(MinimalViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_link_libraries(MinimalViewer PRIVATE Onyx::Onyx)

if(WIN32)
    set_target_properties(MinimalViewer PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()

# Fonts: the engine's ImGui setup loads fonts from third_party/fonts relative
# to the executable (same convention as GoWToolkit). Copy them next to the exe.
if(NOT APPLE)
    add_custom_command(TARGET MinimalViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/third_party/fonts"
            "$<TARGET_FILE_DIR:MinimalViewer>/third_party/fonts"
        COMMENT "Copying fonts next to MinimalViewer")
endif()

# FFmpeg DLLs (Windows) -- Onyx links ffmpeg_lib, so the runtime DLLs must sit
# beside any Onyx-consuming exe.
if(WIN32 AND FFMPEG_DLLS)
    add_custom_command(TARGET MinimalViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FFMPEG_DLLS} "$<TARGET_FILE_DIR:MinimalViewer>"
        COMMENT "Copying FFmpeg DLLs next to MinimalViewer")
endif()
