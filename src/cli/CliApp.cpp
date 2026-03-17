#include "CliApp.h"
#include "../core/ProfileManager.h"
#include "../core/WadTypes.h"
#include "../core/vfs/OsFile.h"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <map>
#include <functional>

namespace GOW {

int CliApp::Run(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    if (args.empty() || args[0] == "help" || args[0] == "-h" || args[0] == "--help") {
        PrintHelp();
        return 0;
    }

    std::string command = args[0];

    if (command == "extract") {
        return HandleExtract(args);
    } else if (command == "inspect") {
        return HandleInspect(args);
    } else if (command == "parse-wad") {
        return HandleParseWad(args);
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        PrintHelp();
        return 1;
    }
}

void CliApp::PrintHelp() {
    std::cout << "GoWTool CLI\n"
              << "Usage: GoWTool <command> [options]\n\n"
              << "Commands:\n"
              << "  parse-wad <file> --game <gow2|ragnarok>  Parse WAD and print node tree.\n"
              << "  extract <archive> <out_dir>              Extract contents from an ISO or WAD/PAK.\n"
              << "  inspect <file>                           Print internal structure of the given file.\n"
              << "  help                                     Print this help message.\n\n"
              << "Run without arguments to launch the GUI.\n";
}

// Helper: print tree recursively
static void PrintEntryTree(const ParsedEntry& entry, int depth) {
    std::string indent(depth * 2, ' ');
    std::string sizeStr;
    if (entry.size >= 1024 * 1024)
        sizeStr = std::to_string(entry.size / (1024 * 1024)) + " MB";
    else if (entry.size >= 1024)
        sizeStr = std::to_string(entry.size / 1024) + " KB";
    else
        sizeStr = std::to_string(entry.size) + " B";

    std::cout << indent << entry.name 
              << "  [" << entry.schemaType << "]"
              << "  size=" << sizeStr
              << "  off=0x" << std::hex << std::setfill('0') << std::setw(8) << entry.offset << std::dec
              << "\n";
    
    for (const auto& child : entry.children) {
        PrintEntryTree(child, depth + 1);
    }
}

int CliApp::HandleParseWad(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: GoWTool parse-wad <file> [--game gow2|ragnarok]\n";
        return 1;
    }

    std::string inputPath = args[1];
    std::string gameHint;

    // Parse --game flag
    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "--game" && i + 1 < args.size()) {
            gameHint = args[i + 1];
            ++i;
        }
    }

    std::filesystem::path path(inputPath);
    if (!std::filesystem::exists(path)) {
        std::cerr << "[CLI] Error: File does not exist: " << inputPath << "\n";
        return 1;
    }

    // Select profile
    std::shared_ptr<IGameProfile> profile;
    if (!gameHint.empty()) {
        profile = ProfileManager::Get().FindProfileByHint(gameHint);
    }
    if (!profile) {
        profile = ProfileManager::Get().DetectProfileForFile(path);
    }
    if (!profile) {
        std::cerr << "[CLI] Error: Could not detect game profile. Use --game gow2|ragnarok\n";
        return 1;
    }

    std::cout << "[CLI] Profile: " << profile->GetName() << "\n";
    std::cout << "[CLI] Parsing: " << inputPath << "\n\n";

    auto file = std::make_shared<OsFile>(path.string());
    if (!file->IsValid()) {
        std::cerr << "[CLI] Error: Could not open file.\n";
        return 1;
    }

    OpenWad wad;
    wad.filename = path.filename().string();
    wad.fullPath = path.string();
    wad.profile = profile;
    wad.fileSource = file;

    if (!profile->ParseWad(file, wad)) {
        std::cerr << "[CLI] Error: ParseWad failed.\n";
        return 1;
    }

    // Print summary
    std::cout << "=== WAD: " << wad.filename << " ===\n";
    std::cout << "Total entries: " << wad.entries.size() << "\n\n";

    // Print tree
    for (const auto& entry : wad.entries) {
        PrintEntryTree(entry, 0);
    }

    // Print type summary
    std::map<std::string, int> typeCounts;
    std::function<void(const ParsedEntry&)> countTypes = [&](const ParsedEntry& e) {
        typeCounts[e.schemaType]++;
        for (const auto& c : e.children) countTypes(c);
    };
    for (const auto& e : wad.entries) countTypes(e);

    std::cout << "\n=== Type Summary ===\n";
    for (const auto& [type, count] : typeCounts) {
        std::cout << "  " << std::setw(30) << std::left << type << " x" << count << "\n";
    }

    return 0;
}

int CliApp::HandleExtract(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Usage: GoWTool extract <archive> <out_dir>\n";
        return 1;
    }
    
    std::string inputPath = args[1];
    std::string outDir = args[2];
    
    std::cout << "[CLI] Preparing to extract: " << inputPath << " to " << outDir << "\n";
    
    std::filesystem::path path(inputPath);
    if (!std::filesystem::exists(path)) {
        std::cerr << "[CLI] Error: File does not exist.\n";
        return 1;
    }

    auto profile = ProfileManager::Get().DetectProfileForFile(path);
    if (!profile) {
        std::cerr << "[CLI] Error: Could not detect game profile for this file.\n";
        return 1;
    }

    std::cout << "[CLI] Detected Profile: " << profile->GetName() << "\n";

    OpenWad wad;
    if (path.extension() == ".iso" || path.extension() == ".ISO") {
        auto vfs = profile->MountArchive(path);
        if (vfs) {
            if (profile->LoadFromArchive(vfs, wad)) {
                std::cout << "[CLI] Successfully parsed ISO!\n";
                for (const auto& entry : wad.entries) {
                    std::cout << "  -> Found Entry: " << entry.name << " (" << entry.schemaType << "), Size: " << entry.size << "\n";
                }
            } else {
                std::cerr << "[CLI] Failed to load from ISO archive.\n";
            }
        } else {
            std::cerr << "[CLI] Failed to mount ISO archive.\n";
        }
    } else {
        std::cerr << "[CLI] Extraction for plain WAD/PAK files is not stubbed yet.\n";
    }

    return 0;
}

int CliApp::HandleInspect(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: GoWTool inspect <file>\n";
        return 1;
    }
    
    std::string inputPath = args[1];
    std::cout << "[CLI] Inspecting file: " << inputPath << "\n";
    // TODO: Detect profile, parse file, print NodeInstance
    return 0;
}

} // namespace GOW
