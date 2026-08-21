#include "CliApp.h"
#include "core/harness/AssetHarness.h"
#include "core/modules/Gow2Module.h"
#include "core/modules/GowrModule.h"
#include "core/types/GameTypes.h"

#include <Onyx/Cli/Commands.h>
#include <Onyx/Cli/Render.h>
#include <Onyx/Exchange/GltfExport.h>
#include <Onyx/Modules/Workspace.h>
#include <Onyx/Parsers/SceneNode.h>
#include <Onyx/Types/TypeCatalog.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace Onyx {

namespace {

// glTF export hook for `decode --to gltf`.
//
// Onyx ships `Onyx::Cli::MakeGltfExportFn` for exactly this, and the port
// design assumed it could be called -- but it cannot: the header declaring it
// (Onyx/Cli/Gltf.h) is public while its only definition lives in
// Examples/OnyxCli/Gltf.cpp, which compiles straight into the onyxbox-cli
// executable and into no library at all. A consumer that includes the header
// gets a link error. Onyx_Exchange itself IS linkable, so the five lines the
// example would have supplied are written here instead. Reported upstream as
// an SDK gap; this is not a workaround for a bug in our own build.
Onyx::Cli::SceneExportFn MakeGltfExport() {
    Onyx::Exchange::GltfOptions opts;
    opts.embedBuffers = true;
    opts.includeSkin  = true;
    return [opts](const Parsers::SceneData& scene, const std::filesystem::path& out,
                  std::string& err) {
        return Onyx::Exchange::ExportSceneData(scene, out, opts, err);
    };
}

// One Workspace with both game modules, matching what the GUI composes.
Onyx::Modules::Workspace& GetWorkspace() {
    static Onyx::Modules::Workspace workspace(Onyx::Types::TypeCatalog::Get());
    static bool registered = [] {
        workspace.AddModule(std::make_unique<Onyx::Gowr::GowrModule>());
        workspace.AddModule(std::make_unique<Onyx::Gow2::Gow2Module>());
        return true;
    }();
    (void)registered;
    return workspace;
}

} // namespace

int CliApp::Run(int argc, char** argv) {
    // The older per-type ITypeHandler system (which `inspect` reaches through
    // AssetHarness::Load) resolves its handles from this catalog. The modules
    // registered below mint their own, separately -- see main.cpp.
    Onyx::GameTypes::RegisterGameTypes();

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.push_back(argv[i]);

    if (args.empty() || args[0] == "help" || args[0] == "-h" || args[0] == "--help") {
        PrintHelp();
        return 0;
    }

    // `inspect` is this toolkit's own verb and has no Onyx equivalent: it
    // reports the scene a GoW entry builds -- parts, materials, texture roles,
    // joints -- which is the only way to see what the material stage actually
    // produced. Everything else is generic container work Onyx::Cli::Run
    // already does better than the hand-rolled versions this file used to
    // carry, including ISO input, which the old `extract` could only reach by
    // bypassing the harness entirely.
    if (args[0] == "inspect") return HandleInspect(args);

    return Onyx::Cli::Run(GetWorkspace(), argc, argv, std::cout, std::cerr,
                          MakeGltfExport(), Onyx::Cli::CmdRender);
}

void CliApp::PrintHelp() {
    std::cout
        << "GoWToolkit CLI\n"
        << "Usage: GoWToolkit <command> [options]\n\n"
        << "Toolkit commands:\n"
        << "  inspect <file> <name> [--game gow2|gowr]\n"
        << "        Build the entry's scene and report parts, materials,\n"
        << "        texture roles and joints.\n\n"
        << "Container commands (Onyx; run any of them with no arguments for\n"
        << "its own usage line):\n"
        << "  probe   <file>                     Score each module against the file.\n"
        << "  list    <file> [--json]            Print the parsed entry tree.\n"
        << "  extract <file> <out_dir>           Write every entry to disk.\n"
        << "  decode  <file> <name> [--to gltf --out <path>]\n"
        << "  render  <file> <name> --out <p.png> [--width N] [--height N]\n"
        << "                                     [--views iso,front,top,...]\n\n"
        << "  --game gow2|gowr   Skip probing and open with that module.\n"
        << "  --strict           Exit non-zero when the document has Error diags.\n\n"
        << "Run without arguments to launch the GUI.\n\n"
        << "Examples:\n"
        << "  GoWToolkit probe   PAND01A.WAD\n"
        << "  GoWToolkit list    PAND01A.WAD\n"
        << "  GoWToolkit inspect PAND01A.WAD gohero00\n"
        << "  GoWToolkit render  PAND01A.WAD gohero00 --out hero.png --views iso,front\n";
}

int CliApp::HandleInspect(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Usage: GoWToolkit inspect <wad-file> <entry-name> [--game gow2|gowr]\n";
        std::cerr << "Example: GoWToolkit inspect PAND01A.WAD gohero00\n";
        return Onyx::Cli::kUsage;
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
        return Onyx::Cli::kUsage;
    }

    std::cout << "[CLI] Parsed " << load.container.entries.size() << " top-level entries.\n";
    std::cout << "[CLI] Found: '" << load.entry->name << "' ["
              << Onyx::Types::TypeCatalog::Get().Label(load.entry->typeId) << "]"
              << " size=" << load.entry->source.size
              << " children=" << load.entry->children.size() << "\n";

    if (load.scene->IsEmpty())
        std::cout << "[CLI] Scene built but has no mesh parts (logical/trigger node).\n";

    Onyx::Harness::PrintSceneStats(*load.scene, std::cout);
    return Onyx::Cli::kOk;
}

} // namespace Onyx
