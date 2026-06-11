#include "MediaKind.h"
#include "fonts/SFSymbols.h"

namespace Onyx {

const char* Name(MediaKind kind) {
    switch (kind) {
        case MediaKind::Image:     return "Image";
        case MediaKind::Mesh:      return "Mesh";
        case MediaKind::Material:  return "Material";
        case MediaKind::Skeleton:  return "Skeleton";
        case MediaKind::Animation: return "Animation";
        case MediaKind::Audio:     return "Audio";
        case MediaKind::Video:     return "Video";
        case MediaKind::Script:    return "Script";
        case MediaKind::Map:       return "Map";
        case MediaKind::Shader:    return "Shader";
        case MediaKind::Container: return "Container";
        case MediaKind::Raw:       return "Raw Data";
        case MediaKind::Unknown:   return "Unknown";
    }
    return "Unknown";
}

const char* Icon(MediaKind kind) {
    switch (kind) {
        case MediaKind::Image:     return ICON_SF_PHOTO;
        case MediaKind::Mesh:      return ICON_SF_CUBE_FILL;
        case MediaKind::Material:  return ICON_SF_PAINTPALETTE_FILL;
        case MediaKind::Skeleton:  return ICON_SF_FIGURE_WALK;
        case MediaKind::Animation: return ICON_SF_FIGURE_RUN;
        case MediaKind::Audio:     return ICON_SF_WAVEFORM;
        case MediaKind::Video:     return ICON_SF_PLAY_RECTANGLE_FILL;
        case MediaKind::Script:    return ICON_SF_SCROLL_FILL;
        case MediaKind::Map:       return ICON_SF_MAP_FILL;
        case MediaKind::Shader:    return ICON_SF_FLAME_FILL;
        case MediaKind::Container: return ICON_SF_ARCHIVEBOX_FILL;
        case MediaKind::Raw:       return ICON_SF_DOCUMENT_FILL;
        case MediaKind::Unknown:   return ICON_SF_QUESTIONMARK_CIRCLE;
    }
    return ICON_SF_QUESTIONMARK_CIRCLE;
}

} // namespace Onyx
