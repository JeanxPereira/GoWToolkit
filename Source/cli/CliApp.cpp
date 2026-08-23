#include "CliApp.h"
#include "core/formats/gowr/SmSchema.h"
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

    // `schema` is likewise toolkit-only: it queries the smschema reflection
    // tables extracted from GoWR.exe, which describe the game's own data
    // layouts and have no container to open.
    if (args[0] == "schema") return HandleSchema(args);

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
        << "        texture roles and joints.\n"
        << "  schema [<lib>:<id> | <namespace.TypeName>]\n"
        << "        Query the smschema reflection tables from GoWR.exe.\n"
        << "        --field <name>   Which structs carry a field of this name.\n"
        << "        --check          Verify the generated tables' invariants.\n\n"
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
        << "  GoWToolkit render  PAND01A.WAD gohero00 --out hero.png --views iso,front\n"
        << "  GoWToolkit schema  dctools.Fog\n"
        << "  GoWToolkit schema  --field WetnessAmount\n";
}

namespace {

void PrintStruct(const Onyx::Gowr::SmSchema::Struct& s, std::ostream& out) {
    out << "(" << s.library << ", 0x" << std::hex << s.id << std::dec << ")  "
        << (s.name ? s.name : "<unnamed>");
    if (s.runtimeSize) out << "   runtime size " << s.runtimeSize << " bytes";
    out << "   " << s.fieldCount << " field" << (s.fieldCount == 1 ? "" : "s") << "\n";
    for (uint16_t i = 0; i < s.fieldCount; ++i) {
        const auto& f = s.fields[i];
        out << "    +" << f.offset << "\t" << f.size << "B\t"
            << Onyx::Gowr::SmSchema::TypeName(f.type) << "\t" << f.name << "\n";
    }
}

} // namespace

int CliApp::HandleSchema(const std::vector<std::string>& args) {
    namespace Sm = Onyx::Gowr::SmSchema;

    // No selector: report what the tables hold. This doubles as the usage
    // line, since the numbers say more about what can be asked than a list of
    // flags would.
    if (args.size() < 2) {
        const Sm::Stats st = Sm::GetStats();
        std::cout << "smschema tables (extracted from GoWR.exe)\n"
                  << "  libraries          " << st.libraries << "\n"
                  << "  structs            " << st.structs << "\n"
                  << "  named structs      " << st.namedStructs << "\n"
                  << "  fields             " << st.fields << "\n"
                  << "  distinct field names " << st.distinctFieldNames << "\n\n"
                  << "A struct is addressed by (library, id): the id is library-local,\n"
                  << "so the same number names a different struct in each library.\n\n"
                  << "  GoWToolkit schema <lib>:<id>            one struct\n"
                  << "  GoWToolkit schema <namespace.TypeName>  every struct with that name\n"
                  << "  GoWToolkit schema --field <name>        structs carrying that field\n"
                  << "  GoWToolkit schema --check               verify the tables\n";
        return Onyx::Cli::kOk;
    }

    if (args[1] == "--check") {
        std::vector<std::string> problems;
        if (Sm::Validate(problems)) {
            const Sm::Stats st = Sm::GetStats();
            std::cout << "smschema tables OK: " << st.structs << " structs, "
                      << st.fields << " fields, " << st.libraries << " libraries.\n";
            return Onyx::Cli::kOk;
        }
        std::cerr << "smschema tables have " << problems.size() << " problem(s):\n";
        for (const auto& p : problems) std::cerr << "  " << p << "\n";
        // Onyx's exit codes are kOk / kUsage / kNoModule / kStrictErrors. The
        // selector was fine here -- the DATA is wrong -- so kStrictErrors is
        // the one that means "what you asked for carried errors".
        return Onyx::Cli::kStrictErrors;
    }

    if (args[1] == "--field") {
        if (args.size() < 3) {
            std::cerr << "Usage: GoWToolkit schema --field <field-name>\n";
            return Onyx::Cli::kUsage;
        }
        const auto hits = Sm::FindStructsWithField(args[2].c_str());
        if (hits.empty()) {
            std::cout << "No struct carries a field named '" << args[2] << "'.\n";
            return Onyx::Cli::kOk;
        }
        std::cout << hits.size() << " struct(s) carry '" << args[2] << "':\n";
        for (const auto* s : hits) {
            const Sm::Field* f = nullptr;
            for (uint16_t i = 0; i < s->fieldCount; ++i)
                if (args[2] == s->fields[i].name) { f = &s->fields[i]; break; }
            std::cout << "  (" << s->library << ", 0x" << std::hex << s->id << std::dec
                      << ")  " << (s->name ? s->name : "<unnamed>");
            if (f) std::cout << "   +" << f->offset << " " << Sm::TypeName(f->type);
            std::cout << "\n";
        }
        return Onyx::Cli::kOk;
    }

    // "<library>:<id>" -- id in decimal or 0x-prefixed hex.
    const std::string& sel = args[1];
    const size_t colon = sel.find(':');
    if (colon != std::string::npos) {
        try {
            const int lib = std::stoi(sel.substr(0, colon), nullptr, 0);
            const int id  = std::stoi(sel.substr(colon + 1), nullptr, 0);
            const Sm::Struct* s = Sm::FindStruct(static_cast<uint16_t>(lib),
                                                 static_cast<uint16_t>(id));
            if (!s) {
                std::cerr << "No struct (" << lib << ", " << id << ") in the tables.\n";
                return Onyx::Cli::kUsage;
            }
            PrintStruct(*s, std::cout);
            return Onyx::Cli::kOk;
        } catch (const std::exception&) {
            std::cerr << "Could not read '" << sel << "' as <library>:<id>.\n";
            return Onyx::Cli::kUsage;
        }
    }

    const auto named = Sm::FindStructsNamed(sel.c_str());
    if (named.empty()) {
        std::cerr << "No struct named '" << sel << "'. Only " << Sm::GetStats().namedStructs
                  << " of " << Sm::GetStats().structs << " structs have a recovered name;\n"
                  << "the rest are addressed by <library>:<id>.\n";
        return Onyx::Cli::kUsage;
    }
    // The same type is registered by several libraries (every animation node
    // library re-declares dctools.AnimNode), so all of them are printed.
    for (size_t i = 0; i < named.size(); ++i) {
        if (i) std::cout << "\n";
        PrintStruct(*named[i], std::cout);
    }
    return Onyx::Cli::kOk;
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
