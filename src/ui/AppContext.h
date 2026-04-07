#pragma once

class AssetDatabase;
struct ParsedEntry;

namespace GOW {
    class DocumentWindow;
    class ViewerRegistry;
}
class AppConfig;

// Shared context passed to all panels via IPanel::draw(AppContext&).
struct AppContext {
    AssetDatabase&        db;
    ParsedEntry*&         selected;
    GOW::DocumentWindow&  documentWindow;
    GOW::ViewerRegistry&  viewerRegistry;
    AppConfig*            config;
};
