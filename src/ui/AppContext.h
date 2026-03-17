#pragma once

class AssetDatabase;
struct ParsedEntry;

namespace GOW {
    class DocumentWindow;
    class ViewerRegistry;
}
class AppConfig;

// Shared context passed to all panels via IPanel::draw(AppContext&).
// Replaces the old pattern of panels accessing extern App* g_App.
struct AppContext {
    AssetDatabase&        db;
    ParsedEntry*&         selected;
    GOW::DocumentWindow&  documentWindow;
    GOW::ViewerRegistry&  viewerRegistry;
    AppConfig*            config;
};
