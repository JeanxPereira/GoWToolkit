#include "CliApp.h"
#include "../core/ProfileManager.h"
#include "../core/WadTypes.h"
#include "../core/vfs/OsFile.h"
#include "../core/types/TypeRegistry.h"
#include "../core/types/ITypeHandler.h"
#include "../core/parsers/shared/SceneNode.h"
#include <filesystem>
#include <fstream>
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

    if (command == "parse-wad") {
        return HandleParseWad(args);
    } else if (command == "inspect") {
        return HandleInspect(args);
    } else if (command == "extract") {
        return HandleExtract(args);
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
        << "  parse-wad <file> [--game gow2|ragnarok]   Parse WAD and print node tree.\n"
        << "  inspect   <file> <name> [--game ...]      Parse and dump mesh/scene stats for a named entry.\n"
        << "  extract   <archive> <out_dir>             Extract all WADs from ISO.\n"
        << "  help                                      Print this help message.\n\n"
        << "Run without arguments to launch the GUI.\n\n"
        << "Examples:\n"
        << "  GoWTool parse-wad PAND01A.WAD\n"
        << "  GoWTool inspect PAND01A.WAD gohero00\n"
        << "  GoWTool inspect game.iso ATHN01.WAD/gohero00\n";
}

// ── Helpers ─────────────────────────────────────────────────────────────────

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

    for (const auto& child : entry.children)
        PrintEntryTree(child, depth + 1);
}

static bool OpenWadFromFile(const std::filesystem::path& path,
                             const std::string& gameHint,
                             OpenWad& wad,
                             std::shared_ptr<IFile>& fileOut)
{
    auto profile = gameHint.empty()
        ? ProfileManager::Get().DetectProfileForFile(path)
        : ProfileManager::Get().FindProfileByHint(gameHint);
    if (!profile) {
        std::cerr << "[CLI] Could not detect game profile. Use --game gow2|ragnarok\n";
        return false;
    }
    std::cout << "[CLI] Profile: " << profile->GetName() << "\n";

    if (!std::filesystem::is_regular_file(path)) {
        std::cerr << "[CLI] Path is not a regular file: " << path << "\n";
        return false;
    }

    auto file = std::make_shared<OsFile>(path.string());
    if (!file->IsValid()) { std::cerr << "[CLI] Cannot open file.\n"; return false; }
    fileOut = file;

    wad.filename = path.filename().string();
    wad.fullPath = path.string();
    wad.profile = profile;
    wad.fileSource = file;

    if (!profile->ParseWad(file, wad)) {
        std::cerr << "[CLI] ParseWad failed.\n";
        return false;
    }
    return true;
}

// Find entry by exact name (depth-first search)
static const ParsedEntry* FindEntryByName(const std::vector<ParsedEntry>& entries, const std::string& name) {
    for (const auto& e : entries) {
        if (e.name == name) return &e;
        if (auto f = FindEntryByName(e.children, name)) return f;
    }
    return nullptr;
}

static void PrintSceneStats(const SceneData& scene) {
    std::cout << "\n=== Scene Data ===\n";
    std::cout << "  Mesh parts  : " << scene.meshParts.size() << "\n";
    std::cout << "  Materials   : " << scene.materials.size() << "\n";
    std::cout << "  Textures    : " << scene.textures.size() << "\n";
    std::cout << "  Has skeleton: " << (scene.HasSkeleton() ? "yes" : "no") << "\n";
    std::cout << "  Is sky      : " << (scene.isSky ? "yes" : "no") << "\n";

    if (scene.skeleton) {
        std::cout << "  Joints      : " << scene.skeleton->joints.size() << "\n";
    }

    int totalVerts = 0, totalIdx = 0;
    for (size_t i = 0; i < scene.meshParts.size(); ++i) {
        const auto& p = scene.meshParts[i];
        totalVerts += (int)p.vertices.size();
        totalIdx   += (int)p.indices.size();
        std::cout << "  Part[" << i << "] '" << p.name << "'"
                  << " matId=" << p.materialId
                  << " layer=" << p.textureLayer
                  << " verts=" << p.vertices.size()
                  << " tris=" << p.indices.size() / 3
                  << "\n";
    }
    std::cout << "  TOTAL: " << totalVerts << " verts, " << totalIdx / 3 << " tris\n";

    for (size_t i = 0; i < scene.materials.size(); ++i) {
        const auto& mat = scene.materials[i];
        std::cout << "  Mat[" << i << "] layers=" << mat.layers.size() << "\n";
        for (size_t j = 0; j < mat.layers.size(); ++j) {
            const auto& l = mat.layers[j];
            std::cout << "    Layer[" << j << "] tex='" << l.textureName
                      << "' hasTexture=" << l.hasTexture << "\n";
        }
    }

    for (size_t i = 0; i < scene.textures.size(); ++i) {
        for (size_t j = 0; j < scene.textures[i].size(); ++j) {
            auto& td = scene.textures[i][j];
            if (td)
                std::cout << "  Tex[" << i << "][" << j << "] " << td->width << "x" << td->height
                          << (td->IsValid() ? " OK" : " INVALID") << "\n";
            else
                std::cout << "  Tex[" << i << "][" << j << "] null\n";
        }
    }
}

// ── Commands ─────────────────────────────────────────────────────────────────

int CliApp::HandleParseWad(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: GoWTool parse-wad <file> [--game gow2|ragnarok]\n";
        return 1;
    }

    std::string gameHint;
    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "--game" && i + 1 < args.size()) {
            gameHint = args[++i];
        }
    }

    std::filesystem::path path(args[1]);
    if (!std::filesystem::is_regular_file(path)) {
        std::cerr << "[CLI] File not found: " << args[1] << "\n";
        return 1;
    }

    OpenWad wad;
    std::shared_ptr<IFile> file;
    if (!OpenWadFromFile(path, gameHint, wad, file))
        return 1;

    std::cout << "\n=== WAD: " << wad.filename << " ===\n";
    std::cout << "Total top-level entries: " << wad.entries.size() << "\n\n";

    for (const auto& e : wad.entries)
        PrintEntryTree(e, 0);

    // Type summary
    std::map<std::string, int> typeCounts;
    std::function<void(const ParsedEntry&)> countTypes = [&](const ParsedEntry& e) {
        typeCounts[e.schemaType]++;
        for (const auto& c : e.children) countTypes(c);
    };
    for (const auto& e : wad.entries) countTypes(e);

    std::cout << "\n=== Type Summary ===\n";
    for (const auto& [type, count] : typeCounts)
        std::cout << "  " << std::setw(30) << std::left << type << " x" << count << "\n";

    return 0;
}

int CliApp::HandleInspect(const std::vector<std::string>& args) {
    // Usage: inspect <file> <entry-name> [--game ...]
    // file can be a plain WAD, or ISO:WAD/entry notation
    if (args.size() < 3) {
        std::cerr << "Usage: GoWTool inspect <wad-file> <entry-name> [--game gow2|ragnarok]\n";
        std::cerr << "Example: GoWTool inspect PAND01A.WAD gohero00\n";
        return 1;
    }

    std::string gameHint;
    for (size_t i = 3; i < args.size(); ++i) {
        if (args[i] == "--game" && i + 1 < args.size())
            gameHint = args[++i];
    }

    std::filesystem::path path(args[1]);
    if (!std::filesystem::is_regular_file(path)) {
        std::cerr << "[CLI] File not found: " << args[1] << "\n";
        return 1;
    }

    const std::string& entryName = args[2];

    OpenWad wad;
    std::shared_ptr<IFile> file;
    if (!OpenWadFromFile(path, gameHint, wad, file))
        return 1;

    std::cout << "[CLI] Parsed WAD with " << wad.entries.size() << " top-level entries.\n";
    std::cout << "[CLI] Looking for entry: '" << entryName << "'\n";

    const ParsedEntry* entry = FindEntryByName(wad.entries, entryName);
    if (!entry) {
        std::cerr << "[CLI] Entry '" << entryName << "' not found.\n";
        std::cerr << "[CLI] Top-level entries:\n";
        for (const auto& e : wad.entries)
            std::cerr << "  " << e.name << " [" << e.schemaType << "]\n";
        return 1;
    }

    std::cout << "[CLI] Found: '" << entry->name << "' [" << entry->schemaType << "]"
              << " size=" << entry->size << " offset=0x" << std::hex << entry->offset << std::dec
              << " children=" << entry->children.size() << "\n";

    auto* handler = TypeRegistry::Get().Resolve(entry->typeId);
    if (!handler) {
        std::cerr << "[CLI] No handler registered for typeId=" << (int)entry->typeId
                  << " (" << entry->schemaType << ")\n";
        return 1;
    }

    std::cout << "[CLI] Handler: " << handler->GetName() << "\n";
    std::cout << "[CLI] Building scene data...\n";

    auto scene = handler->BuildSceneData(*entry, wad);
    if (!scene) {
        std::cerr << "[CLI] BuildSceneData returned null. Check LOG output above.\n";
        return 1;
    }

    if (scene->IsEmpty()) {
        std::cout << "[CLI] Scene built but has no mesh parts (logical/trigger node).\n";
    }

    PrintSceneStats(*scene);
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

    auto profile = ProfileManager::Get().DetectProfileForFile(isoPath);
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

    OpenWad topWad;
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

} // namespace GOW
