#include "CliApp.h"
#include "cli/RenderCommand.h"
#include "core/harness/AssetHarness.h"
#include <Onyx/Services/ProfileManager.h>
#include "core/WadTypes.h"
#include <Onyx/Vfs/OsFile.h>
#include <Onyx/Types/TypeRegistry.h>
#include <Onyx/Types/ITypeHandler.h>
#include <Onyx/Types/TypeCatalog.h>
#include "core/types/GameTypes.h"
#include <Onyx/Parsers/SceneNode.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <functional>

namespace Onyx {

int CliApp::Run(int argc, char** argv) {
    // Populate the type catalog before any parse Ã¢â‚¬â€ handles are invalid otherwise.
    Onyx::GameTypes::RegisterGameTypes();

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    if (args.empty() || args[0] == "help" || args[0] == "-h" || args[0] == "--help") {
        PrintHelp();
        return 0;
    }

    std::string command = args[0];

    if (command == "parse-wad") {
        return HandleParseWad(args);
    } else if (command == "inspect") {
        return HandleInspect(args);
    } else if (command == "extract") {
        return HandleExtract(args);
    } else if (command == "render") {
        return Onyx::Cli::RunRenderCommand(args);
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        PrintHelp();
        return 1;
    }
}

void CliApp::PrintHelp() {
    std::cout
        << "GoWTool CLI\n"
        << "Usage: GoWTool <command> [options]\n\n"
        << "Commands:\n"
        << "  parse-wad <file> [--game gow2|gowr]       Parse WAD and print node tree.\n"
        << "  inspect   <file> <name> [--game ...]      Parse and dump mesh/scene stats for a named entry.\n"
        << "  render    <file> <name> [options]         Draw the entry offscreen to a PNG (--help for options).\n"
        << "  extract   <archive> <out_dir>             Extract all WADs from ISO.\n"
        << "  help                                      Print this help message.\n\n"
        << "Run without arguments to launch the GUI.\n\n"
        << "Examples:\n"
        << "  GoWTool parse-wad PAND01A.WAD\n"
        << "  GoWTool inspect PAND01A.WAD gohero00\n"
        << "  GoWTool render GOW.wad goProtoHeroA00 --angles --report hero.json\n";
}

// Ã¢â€â‚¬Ã¢â€â‚¬ Helpers Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

static void PrintEntryTree(const AssetEntry& entry, int depth) {
    std::string indent(depth * 2, ' ');
    std::string sizeStr;
    if (entry.size >= 1024 * 1024)
        sizeStr = std::to_string(entry.size / (1024 * 1024)) + " MB";
    else if (entry.size >= 1024)
        sizeStr = std::to_string(entry.size / 1024) + " KB";
    else
        sizeStr = std::to_string(entry.size) + " B";

    std::cout << indent << entry.name
              << "  [" << Onyx::Types::TypeCatalog::Get().Label(entry.typeId) << "]"
              << "  size=" << sizeStr
              << "  off=0x" << std::hex << std::setfill('0') << std::setw(8) << entry.offset << std::dec
              << "\n";

    for (const auto& child : entry.children)
        PrintEntryTree(child, depth + 1);
}

// OpenWadFromFile is gone: Harness::LoadContainer does the same walk and
// additionally calls IAssetProfile::PrepareForParse, which the GUI always
// calls and this file never did.

// FindEntryByName and PrintSceneStats moved to core/harness/AssetHarness
// so `inspect`, `render` and any future headless consumer report the same
// facts from the same walk.

// Ã¢â€â‚¬Ã¢â€â‚¬ Commands Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

int CliApp::HandleParseWad(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: GoWTool parse-wad <file> [--game gow2|gowr]\n";
        return 1;
    }

    std::string gameHint;
    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "--game" && i + 1 < args.size()) {
            gameHint = args[++i];
        }
    }

    Onyx::Harness::LoadRequest req{std::filesystem::path(args[1]), "", gameHint};
    Onyx::Harness::LoadResult  load;
    if (!Onyx::Harness::LoadContainer(req, load)) {
        std::cerr << "[CLI] " << load.error << "\n";
        return 1;
    }
    const AssetContainer& wad = load.container;

    std::cout << "[CLI] Profile: "
              << (wad.profile ? wad.profile->GetName() : "(none)") << "\n";
    std::cout << "\n=== WAD: " << wad.filename << " ===\n";
    std::cout << "Total top-level entries: " << wad.entries.size() << "\n\n";

    for (const auto& e : wad.entries)
        PrintEntryTree(e, 0);

    // Type summary
    std::map<std::string, int> typeCounts;
    std::function<void(const AssetEntry&)> countTypes = [&](const AssetEntry& e) {
        typeCounts[Onyx::Types::TypeCatalog::Get().Label(e.typeId)]++;
        for (const auto& c : e.children) countTypes(c);
    };
    for (const auto& e : wad.entries) countTypes(e);

    std::cout << "\n=== Type Summary ===\n";
    for (const auto& [type, count] : typeCounts)
        std::cout << "  " << std::setw(30) << std::left << type << " x" << count << "\n";

    return 0;
}

int CliApp::HandleInspect(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Usage: GoWTool inspect <wad-file> <entry-name> [--game gow2|gowr]\n";
        std::cerr << "Example: GoWTool inspect PAND01A.WAD gohero00\n";
        return 1;
    }

    std::string gameHint;
    for (size_t i = 3; i < args.size(); ++i) {
        if (args[i] == "--game" && i + 1 < args.size())
            gameHint = args[++i];
    }

    Onyx::Harness::LoadRequest req{std::filesystem::path(args[1]), args[2], gameHint};
    Onyx::Harness::LoadResult  load;

    if (!Onyx::Harness::Load(req, load)) {
        std::cerr << "[CLI] " << load.error << "\n";
        if (!load.container.entries.empty() && !load.entry) {
            std::cerr << "[CLI] Top-level entries:\n";
            for (const auto& e : load.container.entries)
                std::cerr << "  " << e.name << " ["
                          << Onyx::Types::TypeCatalog::Get().Label(e.typeId) << "]\n";
        }
        return 1;
    }

    std::cout << "[CLI] Profile: "
              << (load.container.profile ? load.container.profile->GetName() : "(none)") << "\n";
    std::cout << "[CLI] Parsed " << load.container.entries.size() << " top-level entries.\n";
    std::cout << "[CLI] Found: '" << load.entry->name << "' ["
              << Onyx::Types::TypeCatalog::Get().Label(load.entry->typeId) << "]"
              << " size=" << load.entry->size
              << " children=" << load.entry->children.size() << "\n";

    if (load.scene->IsEmpty())
        std::cout << "[CLI] Scene built but has no mesh parts (logical/trigger node).\n";

    Onyx::Harness::PrintSceneStats(*load.scene, std::cout);
    return 0;
}

int CliApp::HandleExtract(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Usage: GoWTool extract <iso-file> <out_dir>\n";
        return 1;
    }

    std::filesystem::path isoPath(args[1]);
    std::filesystem::path outDir(args[2]);

    if (!std::filesystem::is_regular_file(isoPath)) {
        std::cerr << "[CLI] File not found: " << args[1] << "\n";
        return 1;
    }

    auto profile = Onyx::Services::ProfileManager::Get().DetectProfileForFile(isoPath);
    if (!profile) {
        std::cerr << "[CLI] Could not detect game profile.\n";
        return 1;
    }

    std::cout << "[CLI] Profile: " << profile->GetName() << "\n";

    auto vfs = profile->MountArchive(isoPath);
    if (!vfs) {
        std::cerr << "[CLI] Failed to mount archive.\n";
        return 1;
    }

    std::filesystem::create_directories(outDir);

    AssetContainer topWad;
    if (!profile->LoadFromArchive(vfs, topWad)) {
        std::cerr << "[CLI] Failed to enumerate ISO contents.\n";
        return 1;
    }

    std::cout << "[CLI] ISO contains " << topWad.entries.size() << " WAD entries.\n";
    int extracted = 0;

    for (const auto& e : topWad.entries) {
        auto childFile = vfs->OpenFile(e.name);
        if (!childFile || !childFile->IsValid()) continue;

        std::filesystem::path outPath = outDir / e.name;
        std::ofstream out(outPath, std::ios::binary);
        if (!out) { std::cerr << "[CLI] Cannot write: " << outPath << "\n"; continue; }

        std::vector<uint8_t> buf(e.size);
        childFile->Seek(0, SEEK_SET);
        childFile->Read(buf.data(), e.size);
        out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
        std::cout << "  Extracted: " << e.name << " (" << e.size << " bytes)\n";
        ++extracted;
    }

    std::cout << "[CLI] Extracted " << extracted << " files to " << outDir << "\n";
    return 0;
}

} // namespace Onyx
