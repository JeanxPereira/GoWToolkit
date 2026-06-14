#pragma once
// Onyx SDK — umbrella public header. Apps may include this for the full
// public surface, or include individual <Onyx/Subsystem/Header.h> directly.

// Core data
#include <Onyx/Vfs/IFile.h>
#include <Onyx/Vfs/IsoFileSystem.h>
#include <Onyx/Vfs/OsFile.h>
#include <Onyx/Vfs/MemoryFile.h>
#include <Onyx/Schema/StructDef.h>
#include <Onyx/Schema/AssetReader.h>
#include <Onyx/Types/TypeId.h>
#include <Onyx/Types/TypeCatalog.h>
#include <Onyx/Types/GameVersion.h>
#include <Onyx/Domain/MediaKind.h>
#include <Onyx/Domain/Entry.h>
#include <Onyx/Domain/Wad.h>
#include <Onyx/Domain/IAssetProfile.h>

// Services
#include <Onyx/Services/AssetDatabase.h>
#include <Onyx/Services/ProfileManager.h>
#include <Onyx/Services/AppConfig.h>
#include <Onyx/Services/Logger.h>
#include <Onyx/Services/Threading.h>
#include <Onyx/Api/ToolkitApi.h>

// App shell + viewers
#include <Onyx/App/Window.h>
#include <Onyx/App/App.h>
#include <Onyx/App/IPanel.h>
#include <Onyx/App/ViewerRegistry.h>
#include <Onyx/Viewers/IDocumentContent.h>
#include <Onyx/Viewers/DocumentWindow.h>
