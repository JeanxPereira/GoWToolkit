#include "core/loaders/GOWRLoaders.h"
#include <Onyx/Types/TypeRegistry.h>
#include "core/WadTypes.h"
#include "core/types/TextureRoles.h"
#include <Onyx/Schema/AssetReader.h>
#include "core/formats/GOWRMeshDefnFormat.h"
#include "core/parsers/gowr/MeshParser.h"
#include "core/parsers/gowr/LodPackIndex.h"
#include "core/parsers/gowr/TexPackIndex.h"
#include "core/parsers/gowr/ProtoParser.h"
#include "core/parsers/gowr/MgParser.h"
#include "core/parsers/gowr/ShaderParser.h"
#include "core/parsers/gowr/MaterialParser.h"
#include "core/parsers/gowr/TextureDecode.h"
#include "core/shaders/DxilDisassembler.h"
#include "ui/CodeView.h"
#include <Onyx/Parsers/SceneNode.h>
#include <Onyx/Viewers/ImageViewer.h>
#include <Onyx/Services/Logger.h>
#include <Onyx/Vfs/SliceFile.h>
#include <Onyx/Vfs/MemoryFile.h>
#include <Onyx/Viewers/Viewport3D.h>
#include <Onyx/Rendering/SceneRendererVk.h>
#include <imgui.h>
#include <algorithm>
#include <string>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <fstream>
#include "core/profiles/gowr/GowrTaxonomy.h"
#include <Onyx/Services/PathUtils.h>


// Onyx v1.1 removed AssetEntry::profileTag; role is reclassified on demand
// from the entry's own name + size via Gowr::Classify().
static Onyx::Gowr::WadEntryRole GetRole(const AssetEntry& e) {
    return Onyx::Gowr::Classify(e).role;
}

#include <Onyx/Services/PathUtils.h>

// ── GOWRLoaders.cpp ────────────────────────────────────────────────────────

namespace Onyx {

// ── Search candidates for runtime files (config.ini, lodpacks.txt) ─────────
static std::vector<std::filesystem::path> ResourceSearchDirs() {
    std::vector<std::filesystem::path> candidates;
    candidates.push_back(std::filesystem::current_path());

    // O writer salva config.ini em PathUtils::getExecutableDir(), entao o reader
    // precisa olhar la tambem. Antes so o CWD era pesquisado, o que perdia o
    // config recem-salvo sempre que o exe era lancado de outro diretorio
    // (ex.: build-ninja\GoWToolkit.exe chamado a partir da raiz do repo).
    // getResourceDir() resolve sozinho o caso do bundle .app no macOS.
    try {
        auto exeDir = PathUtils::getExecutableDir();
        candidates.push_back(exeDir);

        auto resDir = PathUtils::getResourceDir();
        if (resDir != exeDir) candidates.push_back(resDir);

        auto up = exeDir.parent_path();
        if (!up.empty() && up != exeDir) candidates.push_back(up);
    } catch (...) {}

    return candidates;
}

static std::filesystem::path FindResource(const std::string& filename) {
    for (const auto& dir : ResourceSearchDirs()) {
        auto p = dir / filename;
        if (std::filesystem::exists(p)) return p;
    }
    return {};
}

// ── Resolve game root from config.ini ─────────────────────────────────────
static std::filesystem::path ReadGameRootFromConfig() {
    auto configPath = FindResource("config.ini");
    if (configPath.empty()) return {};
    ONYX_LOGF_INFO("[GOWRLoaders] Found config.ini at: %s", configPath.string().c_str());

    std::ifstream cfg(configPath);
    if (!cfg.is_open()) return {};

    std::string line;
    std::getline(cfg, line); // skip first line (comment)
    if (!std::getline(cfg, line)) return {};

    // Expect "gameroot=D:\..." or just the path on line 1
    auto eq = line.find('=');
    if (eq != std::string::npos)
        line = line.substr(eq + 1);

    // Trim whitespace and quotes
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '"'))
        line.pop_back();
    while (!line.empty() && (line.front() == ' ' || line.front() == '"'))
        line.erase(0, 1);

    return std::filesystem::path(line);
}

// ── Auto-detect game root from a WAD path ──────────────────────────────────
//
// GOWR layout puts WADs under <gameRoot>/exec/wad/pc_le/. We walk up from the
// WAD's parent dir looking for an ancestor that contains exec/wad/pc_le, so we
// support both the canonical layout (wad sits in pc_le) and one or two levels
// deeper (e.g. user keeps wads in a subfolder).
static std::filesystem::path TryDetectGowrGameRoot(const std::filesystem::path& wadPath) {
    std::error_code ec;
    auto p = std::filesystem::weakly_canonical(wadPath, ec);
    if (ec) p = wadPath;
    p = p.parent_path();

    for (int hops = 0; hops < 6 && !p.empty(); ++hops) {
        std::error_code probe;
        if (std::filesystem::exists(p / "exec" / "wad" / "pc_le", probe)) {
            return p;
        }
        auto parent = p.parent_path();
        if (parent == p) break; // reached filesystem root
        p = parent;
    }
    return {};
}

// Returns true ONLY if config.ini was just written (caller should invalidate
// cached singletons). Returns false if config.ini already existed or detection
// failed.
bool EnsureGowrConfigIni(const std::filesystem::path& wadPath) {
    if (!FindResource("config.ini").empty()) return false;

    auto gameRoot = TryDetectGowrGameRoot(wadPath);
    if (gameRoot.empty()) {
        ONYX_LOGF_DEBUG("[GOWRLoaders] Could not auto-detect GOWR game root from: %s",
                  wadPath.string().c_str());
        return false;
    }

    auto target = std::filesystem::path(PathUtils::getExecutableDir()) / "config.ini";
    std::ofstream out(target, std::ios::trunc);
    if (!out.is_open()) {
        ONYX_LOGF_WARN("[GOWRLoaders] Cannot write config.ini at: %s", target.string().c_str());
        return false;
    }
    out << "; GoWToolkit auto-generated. Game root containing exec/wad/pc_le.\n";
    out << "gameroot=" << gameRoot.string() << "\n";
    out.close();

    ONYX_LOGF_INFO("[GOWRLoaders] Auto-detected game root '%s' (saved to %s)",
             gameRoot.string().c_str(), target.string().c_str());
    return true;
}

// ── LodPackIndex singleton ─────────────────────────────────────────────────

static LodPackIndex* s_lodIndex = nullptr;

static LodPackIndex& GetLodIndex() {
    if (!s_lodIndex) {
        s_lodIndex = new LodPackIndex();
        auto lodpacksTxt = FindResource("lodpacks.txt");
        auto gameRoot    = ReadGameRootFromConfig();

        if (gameRoot.empty()) {
            ONYX_LOGF_WARN("[GOWRLoaders] config.ini not found - LOD lookup disabled");
        } else if (!lodpacksTxt.empty()) {
            ONYX_LOGF_INFO("[GOWRLoaders] Found lodpacks.txt at: %s", lodpacksTxt.string().c_str());
            s_lodIndex->LoadFromList(lodpacksTxt, gameRoot);
        } else {
            // A shipped install carries no lodpacks.txt (it is an artefact of the
            // C# extractor), so index every .lodpack in pc_le directly. Without
            // this the parser falls back to the low-res copy embedded in the WAD.
            s_lodIndex->LoadFromGameRoot(gameRoot);
        }
    }
    return *s_lodIndex;
}

static TexPackIndex* s_texIndex = nullptr;
static std::mutex s_texIndexMutex;
static bool s_texIndexStarted = false;

static void StartTexIndexLoad() {
    auto gameRoot = ReadGameRootFromConfig();
    if (!gameRoot.empty()) {
        s_texIndex->LoadFromGameRoot(gameRoot);
    } else {
        ONYX_LOGF_WARN("[GOWRLoaders] config.ini not found — Texture lookup from texpack disabled");
        s_texIndex->SetLoaded();
    }
}

TexPackIndex& GetTexIndex() {
    std::lock_guard<std::mutex> lock(s_texIndexMutex);
    if (!s_texIndex) {
        s_texIndex = new TexPackIndex();
    }
    if (!s_texIndexStarted) {
        s_texIndexStarted = true;
        // Launch indexing on a background thread — does NOT block UI
        std::thread(StartTexIndexLoad).detach();
    }
    return *s_texIndex;
}

void InvalidateLodIndex() {
    delete s_lodIndex;
    s_lodIndex = nullptr;
    delete s_texIndex;
    s_texIndex = nullptr;
}

// ── GOWR Mesh Handling ─────────────────────────────────────────────────────

// MG_*_gpu names carry a trailing part/LOD index ("_0", "_12") that the go*
// instance names do not: goathena10 pairs with MG_athena10_0_gpu. Strip one
// trailing _<digits> so the two can be compared.
static std::string StripPartIndex(const std::string& s) {
    auto u = s.rfind('_');
    if (u == std::string::npos || u + 1 >= s.size()) return s;
    for (size_t i = u + 1; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return s;
    return s.substr(0, u);
}

// -- GOWR detail-level document ----------------------------------------------
// A GOWR model is a set of parts, each with its own chain of detail levels.
// The renderer draws every batch it is handed, so without a filter all levels
// of every part stack on top of one another.
//
// This wraps the viewport and drives per-batch visibility from one level
// choice, defaulting to LOD 0. The level of each batch comes from the MG part
// table rather than being inferred, so parts with shorter chains clamp to
// their own last level instead of disappearing.
class GowrLodDocument : public Viewers::IDocumentContent {
public:
    // Holds its own copy of the container so a LOD change can rebuild the
    // scene after the WAD that produced it has closed.
    GowrLodDocument(std::shared_ptr<Viewers::Viewport3D> vp,
                    std::shared_ptr<Domain::AssetContainer> wad,
                    Domain::AssetEntry entry, bool attachSkeleton, int maxLevels)
        : m_vp(std::move(vp)), m_wad(std::move(wad)), m_entry(std::move(entry)),
          m_attachSkeleton(attachSkeleton), m_maxLevels(maxLevels) {}

    std::string GetName() const override { return m_vp->GetName(); }
    void Draw() override { Apply(); m_vp->Draw(); }
    Viewers::Viewport3D* GetEmbeddedViewport() override { return m_vp.get(); }

    void DrawInspector() override {
        ImGui::Text("Level of Detail");

        std::vector<std::string> labels;
        labels.reserve(m_maxLevels + 1);
        labels.push_back("All levels");
        for (int i = 0; i < m_maxLevels; ++i)
            labels.push_back("LOD " + std::to_string(i));

        std::vector<const char*> items;
        items.reserve(labels.size());
        for (const auto& s : labels) items.push_back(s.c_str());

        if (ImGui::Combo("##gowr_lod", &m_lod, items.data(), (int)items.size())) {
            m_dirty = true;
            m_vp->RequestRedraw();
        }
        ImGui::TextDisabled("%d levels; parts with shorter chains clamp to their last.",
                            m_maxLevels);
        ImGui::Separator();
        m_vp->DrawInspector();
    }

private:
    // Rebuilds the scene at the chosen level and re-submits it.
    //
    // The previous version reached Viewport3D::GetSceneRenderer() and flipped
    // RenderBatch::isVisible, which v1.1 removed -- the renderer is private
    // and there is no visibility API -- leaving the picker inert. Rebuilding
    // costs a re-decode of the entry (including its textures), which is why
    // it happens on a user's combo change and never per frame.
    void Apply() {
        if (!m_dirty) return;
        m_dirty = false;

        GowrSceneMeta meta;
        const int level = (m_lod == 0) ? kAllLevels : (m_lod - 1);
        auto scene = BuildGowrScene(m_entry, *m_wad, m_attachSkeleton, meta, level);
        if (!scene) {
            ONYX_LOGF_WARN("[GOWRLoaders] LOD %d: rebuild produced no scene", m_lod - 1);
            return;
        }
        m_vp->LoadScene(std::move(scene));
        m_vp->RequestRedraw();
    }

    std::shared_ptr<Viewers::Viewport3D>    m_vp;
    std::shared_ptr<Domain::AssetContainer> m_wad;
    Domain::AssetEntry                      m_entry;
    bool m_attachSkeleton = false;
    int  m_maxLevels = 1;
    int  m_lod       = 1;   // "LOD 0" -- the finest level, matching the default build
    bool m_dirty     = false;
};

// -- Material resolution ------------------------------------------------------
// A material's texture list lives in a separate, unnamed companion entry. It is
// laid down right after the MAT in descriptor order, but the tree the browser
// builds does not preserve that order, so the pairing is made by content: the
// companion lists the material's shader permutations, and those are named after
// the material's own hash. Matching on that proves the pairing rather than
// assuming it.
static void FlattenEntries(const std::vector<AssetEntry>& in,
                           std::vector<const AssetEntry*>& out) {
    for (const auto& e : in) {
        out.push_back(&e);
        if (!e.children.empty()) FlattenEntries(e.children, out);
    }
}

static std::string MaterialHashKey(const std::string& matName) {
    // MAT_DE674F96622453EB -> "de674f96622453eb", which is how its shaders
    // spell it.
    const size_t us = matName.find_last_of('_');
    if (us == std::string::npos) return {};
    std::string h = matName.substr(us + 1);
    for (auto& c : h) c = (char)std::tolower((unsigned char)c);
    return h;
}

// A texture that never streams lives entirely in this WAD, as a descriptor
// entry and a payload entry sharing its name. 26 of r_heroa00's 2798 textures
// are like this, up to 512 pixels, and the texpack knows nothing about them.
// -- Mesh material table ------------------------------------------------------
// A mesh names its materials by index, and the table it indexes is per mesh:
// r_heroa00's MESH_heroa00_0 uses indices 0..97 while the WAD holds 884
// materials, so treating the index as WAD-wide lands almost every part on the
// wrong material.
//
// The table is a reference list holding only MAT_ names, and it sits in the
// descriptor immediately after its mesh - measured on r_heroa00, all 18 lists
// pair that way. Count alone does not identify it: six meshes there declare two
// materials and six lists hold two. Payload order does not either; it agrees
// with descriptor order for only 13 of the 18.
//
// The browser's tree groups entries into folders and loses descriptor order, so
// it is read back from the WAD's own descriptor array. Layout per Wad.md.
struct WadDescriptor {
    std::string name;
    uint16_t    type = 0;
    uint32_t    size = 0;
};

static std::vector<WadDescriptor> ReadWadDescriptors(AssetContainer& wad) {
    std::vector<WadDescriptor> out;
    if (!wad.fileSource) return out;

    constexpr size_t kHeader = 64, kStride = 0x90, kNameOff = 0x18, kNameMax = 56;

    uint32_t magic = 0, version = 0, count = 0;
    wad.fileSource->Seek(0, SEEK_SET);
    wad.fileSource->Read(&magic, 4);
    wad.fileSource->Read(&version, 4);
    wad.fileSource->Read(&count, 4);
    if (magic != 0x434F5457 || count == 0 || count > 1000000) return out;
    if (wad.fileSource->Size() < kHeader + (size_t)count * kStride) return out;

    out.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const size_t rec = kHeader + (size_t)i * kStride;
        WadDescriptor de;
        uint16_t group = 0;
        wad.fileSource->Seek(rec, SEEK_SET);
        wad.fileSource->Read(&group, 2);
        wad.fileSource->Read(&de.type, 2);
        wad.fileSource->Read(&de.size, 4);

        char name[kNameMax + 1] = {};
        wad.fileSource->Seek(rec + kNameOff, SEEK_SET);
        wad.fileSource->Read(name, kNameMax);
        de.name = name;
        out.push_back(std::move(de));
    }
    return out;
}

// A texture that never streams lives entirely in this WAD. It occupies three
// entries sharing one name - payload, descriptor, and a third small record -
// so they must be told apart by WAD type rather than by size: the third one
// ranges from 0x4C to 0x124 bytes and straddles the descriptor's own 0xC8.
constexpr uint16_t kWadTypeTexturePayload    = 0x80A2;
constexpr uint16_t kWadTypeTextureDescriptor = 0x0022;

static bool DecodeTextureFromWad(AssetContainer& wad,
                                 const std::vector<WadDescriptor>& descs,
                                 const std::vector<const AssetEntry*>& flat,
                                 const std::string& name,
                                 Parsers::TextureData& out, std::string& error) {
    uint32_t descSize = 0, paySize = 0;
    for (const auto& de : descs) {
        if (de.name != name) continue;
        if (de.type == kWadTypeTextureDescriptor) descSize = de.size;
        else if (de.type == kWadTypeTexturePayload) paySize = de.size;
    }
    if (descSize == 0 || paySize == 0) {
        error = "not present in this WAD";
        return false;
    }

    const AssetEntry* desc = nullptr;
    const AssetEntry* pay  = nullptr;
    for (const AssetEntry* e : flat) {
        if (e->name != name) continue;
        if (e->source.size == descSize) desc = e;
        if (e->source.size == paySize)  pay  = e;
    }
    if (!desc || !pay) { error = "entries not reachable from the tree"; return false; }

    auto dFile = std::make_shared<Vfs::SliceFile>(wad.fileSource, desc->source.offset, desc->source.size);
    auto pFile = std::make_shared<Vfs::SliceFile>(wad.fileSource, pay->source.offset, pay->source.size);
    return GOWRDecodeResidentTexture(dFile, pFile, name, out, error);
}

// Material names for one mesh, in the order its submeshes index them. Empty
// when the table cannot be identified, which the caller must treat as "no
// materials" rather than falling back to a guess.
static std::vector<std::string> ResolveMeshMaterials(
        AssetContainer& wad, const std::vector<WadDescriptor>& descs,
        const std::vector<const AssetEntry*>& flat,
        const AssetEntry& meshEntry, uint32_t matCount) {
    std::vector<std::string> out;
    if (matCount == 0) return out;

    for (size_t i = 0; i + 1 < descs.size(); ++i) {
        if (descs[i].name != meshEntry.name || descs[i].size != meshEntry.source.size) continue;

        const WadDescriptor& next = descs[i + 1];
        for (const AssetEntry* e : flat) {
            if (e->name != next.name || e->source.size != next.size) continue;

            auto file = std::make_shared<Vfs::SliceFile>(wad.fileSource, e->source.offset, e->source.size);
            std::vector<MatReference> refs;
            if (!GOWRMaterialParseRefs(file, refs)) continue;
            if (refs.size() != matCount) continue;

            std::vector<std::string> names;
            for (const auto& r : refs) {
                if (r.name.rfind("MAT_", 0) != 0) { names.clear(); break; }
                names.push_back(r.name);
            }
            if (names.size() == matCount) return names;
        }
    }
    return out;
}

// Maps every reference list in a WAD to the material it belongs to, in one
// pass. Probing per material instead is quadratic and bites hard on real
// content: r_heroa00 holds 25,207 entries and 884 materials, which is 15.8
// million parses and freezes the app, where r_athena00's 4 materials over 114
// entries never showed the cost.
using MaterialRefIndex = std::unordered_map<std::string, std::shared_ptr<Vfs::IFile>>;

static MaterialRefIndex BuildMaterialRefIndex(
        AssetContainer& wad, const std::vector<const AssetEntry*>& flat) {
    MaterialRefIndex index;

    for (const AssetEntry* e : flat) {
        if (e->source.size < 8 + 76 || e->source.size > (1u << 20)) continue;

        auto file = std::make_shared<Vfs::SliceFile>(wad.fileSource, e->source.offset, e->source.size);
        std::vector<MatReference> refs;
        if (!GOWRMaterialParseRefs(file, refs)) continue;

        // A list belongs to whichever material its shader permutations name.
        for (const auto& r : refs) {
            if (!r.isShader) continue;
            const size_t sep = r.name.find("_ps_");
            if (sep == std::string::npos || sep == 0) continue;
            index.emplace(r.name.substr(0, sep), file);
            break;
        }
    }
    return index;
}
// Builds the render-ready scene for a GOWR mesh/rig entry.
//
// Split out of what used to be SharedGowrMeshLoad (which built the scene AND
// wrapped it in a Viewport3D) because the two callers need different halves:
// the ITypeHandler path wants a viewer, and GowrModule's Scene decoder -- the
// path the Shell actually takes when you click a node -- wants the SceneData
// itself. While that decoder was a stub, every mesh click in the GUI answered
// "decode failed", with 500 lines of working loader sitting unreachable behind
// the other entry point.
//
// Returns null when there is nothing to show. An empty (but non-null) scene
// means "parsed fine, no geometry", which the viewer path renders as an empty
// viewport rather than an error.
std::unique_ptr<Parsers::SceneData> BuildGowrScene(const AssetEntry& entry, AssetContainer& wad,
                                                   bool attachSkeleton, GowrSceneMeta& outMeta,
                                                   int lodLevel) {
    if (!wad.fileSource) return nullptr;

    // ── Slice the MESH file ────────────────────────────────────────────
    auto meshFile = std::make_shared<Vfs::SliceFile>(
        wad.fileSource, entry.source.offset, entry.source.size);

    // ── Find the paired MG_GPU sibling ────────────────────────────────
    // Strip prefix (MESH_ or MG_) and trailing ---NNNNN hash to get the base name.
    // GO instances are named go<base>; mesh/gpu files are MESH_<base> / MG_<base>.
    const bool isInstance = (entry.typeId == GameTypes::GameObjectInst);
    std::string base = entry.name;
    if (isInstance) {
        if (base.rfind("go", 0) == 0) base = base.substr(2);
    } else {
        for (const char* pfx : {"MESH_", "MG_"}) {
            if (base.rfind(pfx, 0) == 0) { base = base.substr(strlen(pfx)); break; }
        }
    }
    auto dashPos = base.rfind("---");
    if (dashPos != std::string::npos) base = base.substr(0, dashPos);

    std::shared_ptr<Vfs::IFile> gpuFile;

    std::function<const AssetEntry*(const std::vector<AssetEntry>&)> findGpu;
    findGpu = [&](const std::vector<AssetEntry>& entries) -> const AssetEntry* {
        for (const auto& e : entries) {
            if (GetRole(e) == Gowr::WadEntryRole::MeshGpu) {
                std::string gpuBase = e.name;
                if (gpuBase.rfind("MG_", 0) == 0) gpuBase = gpuBase.substr(3);
                auto d = gpuBase.rfind("---");
                if (d != std::string::npos) gpuBase = gpuBase.substr(0, d);
                if (gpuBase.size() > 4 &&
                    gpuBase.substr(gpuBase.size() - 4) == "_gpu")
                    gpuBase = gpuBase.substr(0, gpuBase.size() - 4);
                if (gpuBase == base) return &e;
                // An instance carries no part index, so compare against the
                // gpu base with its trailing _<N> removed. First match wins:
                // when a model ships several LODs we take the lowest.
                if (isInstance && StripPartIndex(gpuBase) == base) return &e;
            }
            if (!e.children.empty()) {
                auto* found = findGpu(e.children);
                if (found) return found;
            }
        }
        return nullptr;
    };

    const AssetEntry* gpuEntry = findGpu(wad.entries);
    if (gpuEntry) {
        gpuFile = std::make_shared<Vfs::SliceFile>(
            wad.fileSource, gpuEntry->source.offset, gpuEntry->source.size);
        ONYX_LOGF_INFO("[GOWRLoaders] GPU: %s (size=%u)",
                 gpuEntry->name.c_str(), gpuEntry->source.size);
    } else if (isInstance) {
        // Most GO instances are not meshes at all (lights, emitters, triggers),
        // so a missing sibling is the normal case rather than a problem.
        ONYX_LOGF_DEBUG("[GOWRLoaders] Instance '%s' has no mesh pair - not renderable",
                  entry.name.c_str());
    } else {
        ONYX_LOGF_WARN("[GOWRLoaders] No MeshGpu sibling for '%s' - hash=0 submeshes only",
                 entry.name.c_str());
    }

    // ── Parse ──────────────────────────────────────────────────────────
    Parsers::MeshData data;
    std::vector<uint32_t> materialOfPart;   // parallel to data.parts
    bool ok = false;

    const LodPackIndex& lodIdx = GetLodIndex();
    if (gpuFile && lodIdx.TotalEntries() > 0) {
        ok = GOWRMeshParser::ParseWithLodPack(meshFile, gpuFile, lodIdx, data, &materialOfPart);
    } else if (gpuFile) {
        ok = GOWRMeshParser::Parse(meshFile, gpuFile, data, &materialOfPart);
    }

    if (!ok || data.parts.empty()) {
        if (!gpuFile && isInstance) {
            ONYX_LOGF_DEBUG("[GOWRLoaders] Nothing to render for instance '%s'", entry.name.c_str());
        } else {
            ONYX_LOGF_WARN("[GOWRLoaders] Parse failed or no parts for '%s'", entry.name.c_str());
        }
        return std::make_unique<Parsers::SceneData>();   // parsed, no geometry
    }

    // ── Find paired MG_<base> file (bone-binding, no _gpu suffix) ─────
    std::shared_ptr<Vfs::IFile> mgFile;
    std::function<const AssetEntry*(const std::vector<AssetEntry>&)> findMg;
    findMg = [&](const std::vector<AssetEntry>& entries) -> const AssetEntry* {
        for (const auto& e : entries) {
            if (GetRole(e) == Gowr::WadEntryRole::MeshDefn &&
                e.name.rfind("MG_", 0) == 0)
            {
                std::string n = e.name.substr(3); // strip "MG_"
                if (n.size() > 4 && n.substr(n.size() - 4) == "_gpu") continue;
                auto d = n.rfind("---");
                if (d != std::string::npos) n = n.substr(0, d);
                if (n == base) return &e;
            }
            if (!e.children.empty()) {
                auto* f = findMg(e.children);
                if (f) return f;
            }
        }
        return nullptr;
    };

    const AssetEntry* mgEntry = findMg(wad.entries);
    if (mgEntry) {
        mgFile = std::make_shared<Vfs::SliceFile>(
            wad.fileSource, mgEntry->source.offset, mgEntry->source.size);
        ONYX_LOGF_INFO("[GOWRLoaders] MG bone-binding: %s (size=%u)",
                 mgEntry->name.c_str(), mgEntry->source.size);
    }

    // ── DIAGNOSTIC: locate paired MDL_<base> and dump first 512 bytes plus
    // a tail sample. Hunting for MAT hash field placement. Remove once link
    // identified.
    {
        std::string mdlBase = base;
        // Strip trailing "_<digits>" suffix (e.g. "athena10_0" → "athena10")
        auto usPos = mdlBase.find_last_of('_');
        if (usPos != std::string::npos && usPos + 1 < mdlBase.size()) {
            bool allDigits = true;
            for (size_t k = usPos + 1; k < mdlBase.size(); ++k) {
                if (!isdigit((unsigned char)mdlBase[k])) { allDigits = false; break; }
            }
            if (allDigits) mdlBase = mdlBase.substr(0, usPos);
        }

        std::function<const AssetEntry*(const std::vector<AssetEntry>&)> findMdl;
        findMdl = [&](const std::vector<AssetEntry>& entries) -> const AssetEntry* {
            for (const auto& e : entries) {
                if (GetRole(e) == Gowr::WadEntryRole::Model && e.name.rfind("MDL_", 0) == 0) {
                    std::string n = e.name.substr(4);
                    auto d = n.rfind("---");
                    if (d != std::string::npos) n = n.substr(0, d);
                    if (n == mdlBase) return &e;
                }
                if (!e.children.empty()) {
                    auto* f = findMdl(e.children);
                    if (f) return f;
                }
            }
            return nullptr;
        };

        if (const AssetEntry* mdlEntry = findMdl(wad.entries)) {
            const uint32_t dumpSz = std::min<uint32_t>(mdlEntry->source.size, 512u);
            std::vector<uint8_t> buf(dumpSz);
            wad.fileSource->Seek(mdlEntry->source.offset, 0);
            wad.fileSource->Read(buf.data(), dumpSz);
            std::string hex; hex.reserve(dumpSz * 3 + 8);
            char tmp[4];
            for (uint32_t b = 0; b < dumpSz; ++b) {
                std::snprintf(tmp, sizeof(tmp), "%02X ", buf[b]);
                hex += tmp;
            }
            ONYX_LOGF_DEBUG("[GOWRLoaders] MDL '%s' size=%u first %u bytes: %s",
                     mdlEntry->name.c_str(), mdlEntry->source.size, dumpSz, hex.c_str());

            // Also dump last 256 bytes if file is larger than dumpSz
            if (mdlEntry->source.size > 512) {
                const uint32_t tailSz = std::min<uint32_t>(mdlEntry->source.size - 512, 256u);
                std::vector<uint8_t> tail(tailSz);
                wad.fileSource->Seek(mdlEntry->source.offset + mdlEntry->source.size - tailSz, 0);
                wad.fileSource->Read(tail.data(), tailSz);
                std::string thex; thex.reserve(tailSz * 3 + 8);
                for (uint32_t b = 0; b < tailSz; ++b) {
                    std::snprintf(tmp, sizeof(tmp), "%02X ", tail[b]);
                    thex += tmp;
                }
                ONYX_LOGF_DEBUG("[GOWRLoaders] MDL tail %u bytes: %s", tailSz, thex.c_str());
            }
        } else {
            ONYX_LOGF_DEBUG("[GOWRLoaders] No MDL_ sibling for '%s' (base='%s')",
                     entry.name.c_str(), mdlBase.c_str());
        }
    }

    // ── Find paired goProto* file (skeleton) ───────────────────────────
    // Skip entirely when the caller asked for a mesh-only view (MESH_ entry).
    // Rigged viewers (goProto*, go*) pass attachSkeleton=true and resolve the
    // rig via name pairing below.
    std::shared_ptr<Parsers::ObjectData> skeleton;
    if (attachSkeleton) {
        // Naming: MESH_athena10_0 → look for goProtoathena10 (strip MESH_/MG_,
        // trailing _N, and trailing ---HASH).
        std::string protoBase = base;
        // Strip trailing "_<digits>" suffix if present (e.g. "athena10_0" → "athena10")
        auto usPos = protoBase.find_last_of('_');
        if (usPos != std::string::npos && usPos + 1 < protoBase.size()) {
            bool allDigits = true;
            for (size_t k = usPos + 1; k < protoBase.size(); ++k) {
                if (!isdigit((unsigned char)protoBase[k])) { allDigits = false; break; }
            }
            if (allDigits) protoBase = protoBase.substr(0, usPos);
        }

        std::function<const AssetEntry*(const std::vector<AssetEntry>&)> findProto;
        findProto = [&](const std::vector<AssetEntry>& entries) -> const AssetEntry* {
            for (const auto& e : entries) {
                if (GetRole(e) == Gowr::WadEntryRole::GameObjectProto) {
                    std::string n = e.name;
                    if (n.rfind("goProto", 0) == 0) n = n.substr(7);
                    auto d = n.rfind("---");
                    if (d != std::string::npos) n = n.substr(0, d);
                    if (n == protoBase) return &e;
                }
                if (!e.children.empty()) {
                    auto* f = findProto(e.children);
                    if (f) return f;
                }
            }
            return nullptr;
        };

        const AssetEntry* protoEntry = findProto(wad.entries);
        if (protoEntry) {
            auto protoFile = std::make_shared<Vfs::SliceFile>(
                wad.fileSource, protoEntry->source.offset, protoEntry->source.size);
            skeleton = GOWRProtoParser::Parse(protoFile);
            if (skeleton) {
                ONYX_LOGF_INFO("[GOWRLoaders] Proto rig '%s': %zu bones",
                         protoEntry->name.c_str(), skeleton->joints.size());
            }
        } else {
            ONYX_LOGF_INFO("[GOWRLoaders] No goProto sibling for '%s' (base='%s')",
                     entry.name.c_str(), protoBase.c_str());
        }
    }

    // -- Resolve parts, detail levels and the bone palette from the MG ------
    // The MG is authoritative for all three: it names which submeshes form
    // each level of each part, and carries the palette that turns a vertex's
    // local bone index into a global skeleton index.
    GOWRMgParser::Data mg;
    std::vector<PartLevel> partMeta(data.parts.size());
    bool haveMg = false;

    if (mgFile) {
        uint32_t meshSubCount = 0;
        for (const auto& p : data.parts)
            if (p.materialId + 1 > meshSubCount) meshSubCount = p.materialId + 1;
        haveMg = GOWRMgParser::Parse(mgFile, meshSubCount, mg);
    }

    if (haveMg) {
        for (size_t i = 0; i < data.parts.size(); ++i) {
            auto&      part = data.parts[i];
            const auto sm   = part.materialId;
            if (sm >= mg.partOfSubmesh.size()) continue;

            const int pi = mg.partOfSubmesh[sm];
            const int lv = mg.levelOfSubmesh[sm];
            partMeta[i] = { pi, lv };
            if (pi < 0) {
                part.name = "sm" + std::to_string(sm) + " (unreferenced)";
                continue;
            }

            char label[64];
            std::snprintf(label, sizeof(label), "Part %02d  LOD %d", pi, lv);
            part.name = label;
        }
    }

    // -- Materials --------------------------------------------------------
    // Each submesh names a material by index; the WAD's material entries are
    // taken in tree order to match. Only the diffuse map is bound for now -
    // normal and occlusion are parsed but the viewport shader has nowhere to
    // put them yet.
    std::vector<const AssetEntry*> flat;
    FlattenEntries(wad.entries, flat);
    const MaterialRefIndex refIndex = BuildMaterialRefIndex(wad, flat);

    // The mesh's own material table decides which MAT each index means.
    uint32_t matCount = 0;
    meshFile->Seek(0x20, SEEK_SET);
    meshFile->Read(&matCount, 4);

    const std::vector<WadDescriptor> wadDescs = ReadWadDescriptors(wad);
    const std::vector<std::string> matNames =
        ResolveMeshMaterials(wad, wadDescs, flat, entry, matCount);

    if (matCount > 0 && matNames.empty()) {
        ONYX_LOGF_WARN("[GOWRLoaders] mesh declares %u materials but its table was "
                 "not found - rendering untextured", matCount);
    }

    std::unordered_map<std::string, const AssetEntry*> matByName;
    for (const AssetEntry* e : flat)
        if (GetRole(*e) == Gowr::WadEntryRole::Material && e->source.size > 0)
            matByName.emplace(e->name, e);

    std::vector<const AssetEntry*> matEntries;
    matEntries.reserve(matNames.size());
    for (const auto& n : matNames) {
        auto it = matByName.find(n);
        matEntries.push_back(it != matByName.end() ? it->second : nullptr);
    }

    // The roles worth decoding, in the order they are reported. Under v1.1 a
    // material binds textures by ROLE into a flat pool -- there is no layer
    // index any more -- so this list is now just "which roles the renderer has
    // a sampler for", not a positional contract. Roles GOWR declares but Onyx
    // cannot sample (Specular, Roughness, Metallic) are filtered by
    // ToSceneRole and reported as skipped.
    static constexpr TextureRole kWantedRoles[] = {
        TextureRole::Diffuse,
        TextureRole::Normal,
        TextureRole::AmbientOcclusion,
        TextureRole::Gloss,
        TextureRole::Height,
        TextureRole::Scatter,
        TextureRole::Detail,
        // Coverage. Onyx v1.2 samples it: a material that ships a mask
        // separately from its diffuse alpha is cut out by it, which is what
        // hair cards and a transparent cornea need to stop rendering solid.
        TextureRole::Opacity,
    };
    constexpr size_t kWantedCount = sizeof(kWantedRoles) / sizeof(kWantedRoles[0]);

    std::vector<Parsers::MaterialDesc> materials(matEntries.size());
    std::vector<std::unique_ptr<Parsers::TextureData>> texturePool;

    for (size_t mi = 0; mi < matEntries.size(); ++mi) {
        const AssetEntry* me = matEntries[mi];
        if (!me) {
            ONYX_LOGF_WARN("[GOWRLoaders] material[%zu] %s: named by the mesh but "
                     "absent from this WAD", mi, matNames[mi].c_str());
            continue;
        }
        auto matFile = std::make_shared<Vfs::SliceFile>(wad.fileSource, me->source.offset, me->source.size);
        auto it      = refIndex.find(MaterialHashKey(me->name));
        auto refFile = (it != refIndex.end()) ? it->second : nullptr;

        GOWRMaterial mat;
        if (!GOWRMaterialParse(matFile, refFile, mat)) continue;
        if (!refFile) {
            ONYX_LOGF_WARN("[GOWRLoaders] material %s: no reference list found", me->name.c_str());
            continue;
        }

        std::string bound;
        for (size_t L = 0; L < kWantedCount; ++L) {
            const MatReference* ref = mat.Texture(kWantedRoles[L]);
            if (!ref) continue;

            const auto sceneRole = ToSceneRole(kWantedRoles[L]);
            if (!sceneRole) continue;   // no sampler -- decoding would be wasted

            auto tex = std::make_unique<Parsers::TextureData>();
            std::string err;
            bool got = GOWRDecodeTexture(GetTexIndex(), ref->textureHash,
                                         ref->name, *tex, err);
            if (!got) {
                // Not in a texpack means it may never stream at all.
                std::string wadErr;
                got = DecodeTextureFromWad(wad, wadDescs, flat, ref->name, *tex, wadErr);
                if (!got) err += "; " + wadErr;
            }
            if (got) {
                if (!bound.empty()) bound += ", ";
                // The NAME, not just the role. A material can declare dozens of
                // textures for one role -- Baldur's head material declares 39,
                // most of them dynamicmaterial region maps -- and knowing that
                // "Diffuse" was bound says nothing about WHICH one won.
                bound += TextureRoleName(kWantedRoles[L]);
                bound += "=";
                bound += ref->name;
                texturePool.push_back(std::move(tex));
                materials[mi].textures[*sceneRole] = static_cast<int>(texturePool.size() - 1);
            } else {
                ONYX_LOGF_WARN("[GOWRLoaders] %s (%s): %s", ref->name.c_str(),
                         TextureRoleName(kWantedRoles[L]), err.c_str());
            }
        }

        std::string skipped;
        for (const auto* tx : mat.Textures()) {
            bool wanted = false;
            for (size_t L = 0; L < kWantedCount; ++L)
                if (kWantedRoles[L] == tx->role) { wanted = true; break; }
            if (wanted) continue;
            if (!skipped.empty()) skipped += ", ";
            skipped += tx->name;
        }

        ONYX_LOGF_INFO("[GOWRLoaders] material[%zu] %s: %zu textures, decoded [%s]",
                 mi, me->name.c_str(), mat.Textures().size(),
                 bound.empty() ? "none" : bound.c_str());
        if (!skipped.empty()) {
            ONYX_LOGF_DEBUG("[GOWRLoaders]   not a mapped role: %s", skipped.c_str());
        }
        if (!materials[mi].textures.count(Parsers::TextureRole::Diffuse)) {
            // Diffuse is what the viewport samples, so say plainly why the
            // model will render untextured rather than leaving it a mystery.
            const MatReference* diff = mat.Texture(TextureRole::Diffuse);
            ONYX_LOGF_WARN("[GOWRLoaders]   no usable diffuse (%s) - this material renders untextured",
                     diff ? diff->name.c_str() : "material declares none");
        }
    }

    // From here on materialId means the material, not the submesh: everything
    // that needed the submesh index has already run.
    for (size_t i = 0; i < data.parts.size(); ++i) {
        const uint32_t mi = (i < materialOfPart.size()) ? materialOfPart[i] : 0u;
        if (mi >= matEntries.size()) {
            ONYX_LOGF_WARN("[GOWRLoaders] part %zu names material %u of %zu",
                     i, mi, matEntries.size());
        }
        data.parts[i].materialId  = mi;
        data.parts[i].textureLayer = 0;
    }

    if (skeleton) {
        // -- Skinning --------------------------------------------------------
        // Vertex bone indices are local to the part's palette, so the jointMap
        // is the palette itself. Parts with no bone semantics stay rigid and
        // bind to their root bone.
        if (haveMg) {
            // Vertex bone indices are global skeleton indices, while the
            // renderer uploads at most 150 matrices per batch. The MG palette
            // is exactly the set of bones a part uses, so it becomes the
            // jointMap and the vertex indices are rebased onto it.
            int skinned = 0, rigid = 0, unmapped = 0;
            size_t offPalette = 0;

            for (size_t i = 0; i < data.parts.size(); ++i) {
                auto&     part = data.parts[i];
                const int pi   = partMeta[i].part;
                if (pi < 0 || pi >= (int)mg.parts.size()) { ++unmapped; continue; }
                const auto& src = mg.parts[pi];

                if (part.isRigid || src.palette.empty()) {
                    part.jointMap = { src.palette.empty() ? src.boneRef
                                                          : src.palette.front() };
                    for (auto& v : part.vertices) {
                        v.boneIndices = glm::uvec4(0, 0, 0, 0);
                        v.boneWeights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                    }
                    ++rigid;
                    continue;
                }

                part.jointMap = src.palette;

                // Global bone index -> slot within this part's palette.
                std::unordered_map<uint32_t, uint32_t> slotOf;
                slotOf.reserve(src.palette.size() * 2);
                for (uint32_t s = 0; s < src.palette.size(); ++s)
                    slotOf.emplace(src.palette[s], s);

                for (auto& v : part.vertices) {
                    for (int k = 0; k < 4; ++k) {
                        auto it = slotOf.find(v.boneIndices[k]);
                        if (it != slotOf.end()) {
                            v.boneIndices[k] = it->second;
                        } else {
                            // Not in the palette: drop the influence rather
                            // than bind the vertex to an arbitrary bone.
                            v.boneIndices[k] = 0;
                            v.boneWeights[k] = 0.0f;
                            ++offPalette;
                        }
                    }
                }
                ++skinned;
            }

            ONYX_LOGF_INFO("[GOWRLoaders] Skinning: %d skinned, %d rigid, %d unmapped parts",
                     skinned, rigid, unmapped);
            if (offPalette > 0) {
                ONYX_LOGF_WARN("[GOWRLoaders] %zu vertex influences referenced a bone "
                         "outside their part's palette", offPalette);
            }
        }

    }

    // One scene for both paths, skeleton or not.
    //
    // The un-skinned branch used to call Viewport3D::LoadFromMeshData, which
    // v1.1 turned into a logged no-op that also clears the scene -- its own
    // comment says "dead code path, no callers", true of the SDK's tree but
    // not of this one. So every GOWR mesh without a rig rendered nothing, and
    // said so only at ONYX_LOGF_WARN. SceneData is the model v1.1 actually
    // renders; a rig-less entry simply has a null skeleton.
    auto scene = std::make_unique<Parsers::SceneData>();
    scene->skeleton  = skeleton;
    scene->flipZ     = true;    // mesh and bones both face -Z; flip once for screen
    scene->meshParts = std::move(data.parts);
    scene->materials = std::move(materials);
    scene->textures  = std::move(texturePool);

    const int levelCount = (haveMg && mg.MaxLevelCount() > 1) ? mg.MaxLevelCount() : 1;

    // ── LOD filter ──────────────────────────────────────────────────────
    // A mesh group's levels are alternative representations of one surface,
    // so keeping them all draws every shell at once -- which is what this
    // loader did, and what "all 5 levels stay visible" in the log meant.
    if (haveMg && levelCount > 1 && lodLevel != kAllLevels) {
        // Highest level each part actually has, so a request past the end of
        // a part's chain shows its coarsest level instead of dropping it.
        std::vector<int> lastOf;
        for (const auto& pl : partMeta) {
            if (pl.part < 0) continue;
            if (static_cast<int>(lastOf.size()) <= pl.part) lastOf.resize(pl.part + 1, -1);
            lastOf[pl.part] = std::max(lastOf[pl.part], pl.level);
        }

        std::vector<Parsers::MeshPart> kept;
        std::vector<PartLevel>         keptMeta;
        kept.reserve(scene->meshParts.size());
        keptMeta.reserve(partMeta.size());

        for (size_t i = 0; i < scene->meshParts.size() && i < partMeta.size(); ++i) {
            const PartLevel& pl = partMeta[i];
            // A part the MG never referenced has no level to filter on; keep
            // it rather than silently dropping geometry we cannot classify.
            const bool keep = (pl.part < 0)
                            || (pl.level == std::min(lodLevel, lastOf[pl.part]));
            if (!keep) continue;
            kept.push_back(std::move(scene->meshParts[i]));
            keptMeta.push_back(pl);
        }

        ONYX_LOGF_INFO("[GOWRLoaders] LOD %d of %d: %zu of %zu parts kept",
                       lodLevel, levelCount, kept.size(), scene->meshParts.size());
        scene->meshParts = std::move(kept);
        partMeta         = std::move(keptMeta);
    }

    outMeta.partLevels = std::move(partMeta);
    outMeta.maxLevels  = levelCount;
    return scene;
}

// Viewer half: build the scene, then wrap it in a viewport (plus the LOD
// document when the mesh group declares more than one level).
static std::shared_ptr<Viewers::IDocumentContent> SharedGowrMeshLoad(const AssetEntry& entry,
                                                                     AssetContainer& wad,
                                                                     bool attachSkeleton) {
    GowrSceneMeta meta;
    auto scene = BuildGowrScene(entry, wad, attachSkeleton, meta);
    if (!scene) return nullptr;

    auto vp = std::make_shared<Viewers::Viewport3D>(entry.name);
    vp->LoadScene(std::move(scene));

    if (meta.maxLevels > 1) {
        // The document keeps its own container copy: a LOD change rebuilds the
        // scene, and `wad` is a reference whose owner may be gone by then.
        auto owned = std::make_shared<Domain::AssetContainer>(wad);
        return std::make_shared<GowrLodDocument>(std::move(vp), std::move(owned),
                                                 entry, attachSkeleton, meta.maxLevels);
    }
    return vp;
}

std::shared_ptr<Schema::AssetNode> GOWRMeshDefnHandler::Parse(std::shared_ptr<Vfs::IFile> file) {
    if (!file || file->Size() < 64) return nullptr;
    GOWRMeshDefnFormat format;
    format.Initialize();
    return Schema::AssetReader::Parse(*format.Root(), file);
}

std::shared_ptr<Viewers::IDocumentContent> GOWRMeshDefnHandler::CreateViewer(const AssetEntry& entry, AssetContainer& wad) {
    // MESH_* is a plain mesh; MG_* is a mesh group, which exists precisely to
    // carry a bone palette. Both are role MeshDefn, so the name is the only
    // thing that distinguishes them -- the same distinction the two former
    // handler classes encoded in their (colliding, therefore unreachable)
    // registrations. attachSkeleton only *attempts* a name-paired goProto*
    // lookup and leaves the skeleton null when there is none, so this is a
    // best-effort request, not an assertion that a rig exists.
    const bool meshGroup = entry.name.rfind("MG_", 0) == 0;
    return SharedGowrMeshLoad(entry, wad, /*attachSkeleton=*/meshGroup);
}
std::shared_ptr<Viewers::IDocumentContent> GOWRMeshGpuHandler::CreateViewer(const AssetEntry& entry, AssetContainer& wad) {
    // MG_*_gpu -- always the rigged half of a mesh group.
    return SharedGowrMeshLoad(entry, wad, /*attachSkeleton=*/true);
}
std::shared_ptr<Viewers::IDocumentContent> GOWRModelInstanceHandler::CreateViewer(const AssetEntry& entry, AssetContainer& wad) {
    // go* instance — load with rig.
    return SharedGowrMeshLoad(entry, wad, /*attachSkeleton=*/true);
}

#include <imgui.h>
#include <Onyx/Viewers/ImageViewer.h>

class GOWRTextureViewer : public Viewers::IDocumentContent {
public:
    GOWRTextureViewer(const AssetEntry& entry) : m_entry(entry), m_name(entry.name) {
        auto lastUs = m_name.find_last_of('_');
        if (lastUs != std::string::npos && lastUs + 1 < m_name.size()) {
            try { m_hash = std::stoull(m_name.substr(lastUs + 1), nullptr, 16); } catch(...) {}
        }
    }

    std::string GetName() const override { return "Tex: " + m_name; }

    void Draw() override {
        auto& texIdx = GetTexIndex();

        // Poll-based lookup: try the cache every frame. As background
        // workers publish more packs, our hash may become available even
        // while overall indexing is still in progress.
        if (!m_initialized) {
            TexpackEntry probe;
            if (texIdx.FindTexture(m_hash, probe)) {
                m_initialized = true;
                FirstLoad(texIdx);
            } else if (!texIdx.IsLoading()) {
                // Indexing finished and the hash is still missing — real fail.
                m_initialized = true;
                m_failReason = "hash not in texpack index";
            } else {
                // Still indexing — show progress and bail until next frame.
                ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x * 0.5f - 150,
                                           ImGui::GetWindowSize().y * 0.5f - 20));
                ImGui::Text("Indexing texpacks in background (parallel)...");
                ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.5f - 150);
                ImGui::ProgressBar(texIdx.GetLoadProgress(), ImVec2(300, 0));
                return;
            }
        }

        if (!m_failReason.empty()) {
            ImGui::TextColored(ImVec4(1,0.6f,0.6f,1), "Texture load failed: %s", m_failReason.c_str());
            ImGui::Text("Name: %s", m_name.c_str());
            ImGui::Text("Hash: 0x%016llx", (unsigned long long)m_hash);
            if (m_texW || m_texH)   ImGui::Text("Size: %ux%u", m_texW, m_texH);
            return;
        }

        if (m_realViewer) m_realViewer->Draw();
    }

private:
    AssetEntry m_entry;
    std::string m_name;
    uint64_t    m_hash = 0;
    bool        m_initialized = false;
    std::string m_failReason;

    std::shared_ptr<Viewers::ImageViewer> m_realViewer;

    uint32_t  m_texW = 0, m_texH = 0;

    void FirstLoad(TexPackIndex& texIdx) {
        auto texData = std::make_unique<Parsers::TextureData>();
        if (!GOWRDecodeTexture(texIdx, m_hash, m_name, *texData, m_failReason)) return;

        m_texW = texData->width;
        m_texH = texData->height;
        m_realViewer = std::make_shared<Viewers::ImageViewer>(m_name, std::move(texData));
    }
};

std::shared_ptr<Viewers::IDocumentContent> GOWRTextureHandler::CreateViewer(const AssetEntry& entry, AssetContainer& wad) {
    return std::make_shared<GOWRTextureViewer>(entry);
}

// A goProto* entry carries the rig, not the geometry: the mesh lives in a
// separate MESH_<base>* (or MG_<base>*) entry paired by name.
//
// Split out of GOWRRigHandler::CreateViewer so BuildSceneData can reach the
// same resolution. It used to live inside the viewer path only, which is why
// `inspect` could never report a proto: the handlers implemented CreateViewer
// and nothing else, so ITypeHandler::BuildSceneData's default nullptr was the
// answer for every GOWR entry, and the CLI reported it as a failure to build.
static const AssetEntry* ResolveProtoMesh(const AssetEntry& entry, AssetContainer& wad) {
    if (!wad.fileSource) return nullptr;

    // Derive the base name: "goProtofox00" → "fox00"
    std::string protoBase = entry.name;
    if (protoBase.rfind("goProto", 0) == 0) protoBase = protoBase.substr(7);
    auto dashPos = protoBase.rfind("---");
    if (dashPos != std::string::npos) protoBase = protoBase.substr(0, dashPos);

    // Find the first MESH_<base>* entry in the WAD
    std::function<const AssetEntry*(const std::vector<AssetEntry>&)> findMesh;
    findMesh = [&](const std::vector<AssetEntry>& entries) -> const AssetEntry* {
        for (const auto& e : entries) {
            if (GetRole(e) == Gowr::WadEntryRole::MeshDefn &&
                e.name.rfind("MESH_", 0) == 0)
            {
                std::string n = e.name.substr(5); // strip "MESH_"
                auto d = n.rfind("---");
                if (d != std::string::npos) n = n.substr(0, d);
                // Strip trailing "_<digits>" LOD suffix
                auto us = n.find_last_of('_');
                if (us != std::string::npos && us + 1 < n.size()) {
                    bool allDigits = true;
                    for (size_t k = us + 1; k < n.size(); ++k)
                        if (!isdigit((unsigned char)n[k])) { allDigits = false; break; }
                    if (allDigits) n = n.substr(0, us);
                }
                if (n == protoBase) return &e;
            }
            if (!e.children.empty()) {
                auto* f = findMesh(e.children);
                if (f) return f;
            }
        }
        return nullptr;
    };

    const AssetEntry* meshEntry = findMesh(wad.entries);
    if (meshEntry) {
        ONYX_LOGF_INFO("[GOWRRigHandler] Found MESH '%s' for proto '%s'",
                 meshEntry->name.c_str(), entry.name.c_str());
        return meshEntry;
    }

    // Fallback: try MG_<base>* (non-gpu) entries
    std::function<const AssetEntry*(const std::vector<AssetEntry>&)> findMg;
    findMg = [&](const std::vector<AssetEntry>& entries) -> const AssetEntry* {
        for (const auto& e : entries) {
            if (GetRole(e) == Gowr::WadEntryRole::MeshDefn &&
                e.name.rfind("MG_", 0) == 0)
            {
                std::string n = e.name.substr(3);
                if (n.size() > 4 && n.substr(n.size() - 4) == "_gpu") {
                    if (!e.children.empty()) { auto* f = findMg(e.children); if (f) return f; }
                    continue;
                }
                auto d = n.rfind("---");
                if (d != std::string::npos) n = n.substr(0, d);
                auto us = n.find_last_of('_');
                if (us != std::string::npos && us + 1 < n.size()) {
                    bool allDigits = true;
                    for (size_t k = us + 1; k < n.size(); ++k)
                        if (!isdigit((unsigned char)n[k])) { allDigits = false; break; }
                    if (allDigits) n = n.substr(0, us);
                }
                if (n == protoBase) return &e;
            }
            if (!e.children.empty()) {
                auto* f = findMg(e.children);
                if (f) return f;
            }
        }
        return nullptr;
    };

    const AssetEntry* mgEntry = findMg(wad.entries);
    if (mgEntry) {
        ONYX_LOGF_INFO("[GOWRRigHandler] Found MG '%s' for proto '%s' (fallback)",
                 mgEntry->name.c_str(), entry.name.c_str());
        return mgEntry;
    }

    ONYX_LOGF_WARN("[GOWRRigHandler] No MESH/MG found for proto '%s' (base='%s')",
             entry.name.c_str(), protoBase.c_str());
    return nullptr;
}

// ── Scene half ────────────────────────────────────────────────────────────
//
// The same four handlers that build a viewer also answer BuildSceneData, so
// the CLI's `inspect` and the GUI's viewport report the same scene from the
// same code. They differ only in what they wrap it in.
//
// LOD is kFinest here, matching BuildGowrScene's own default and what the
// viewer opens with. `inspect` has no picker to change it.

std::unique_ptr<Parsers::SceneData>
GOWRMeshDefnHandler::BuildSceneData(const AssetEntry& entry, AssetContainer& wad) {
    GowrSceneMeta meta;
    const bool meshGroup = entry.name.rfind("MG_", 0) == 0;
    return BuildGowrScene(entry, wad, /*attachSkeleton=*/meshGroup, meta);
}

std::unique_ptr<Parsers::SceneData>
GOWRMeshGpuHandler::BuildSceneData(const AssetEntry& entry, AssetContainer& wad) {
    GowrSceneMeta meta;
    return BuildGowrScene(entry, wad, /*attachSkeleton=*/true, meta);
}

std::unique_ptr<Parsers::SceneData>
GOWRModelInstanceHandler::BuildSceneData(const AssetEntry& entry, AssetContainer& wad) {
    GowrSceneMeta meta;
    return BuildGowrScene(entry, wad, /*attachSkeleton=*/true, meta);
}

std::unique_ptr<Parsers::SceneData>
GOWRRigHandler::BuildSceneData(const AssetEntry& entry, AssetContainer& wad) {
    const AssetEntry* mesh = ResolveProtoMesh(entry, wad);
    if (!mesh) return nullptr;
    GowrSceneMeta meta;
    return BuildGowrScene(*mesh, wad, /*attachSkeleton=*/true, meta);
}

std::shared_ptr<Viewers::IDocumentContent>
GOWRRigHandler::CreateViewer(const AssetEntry& entry, AssetContainer& wad) {
    const AssetEntry* mesh = ResolveProtoMesh(entry, wad);
    if (!mesh) return nullptr;
    return SharedGowrMeshLoad(*mesh, wad, /*attachSkeleton=*/true);
}

ONYX_REGISTER_FILE_TYPE(GOWRMeshDefnHandler);
ONYX_REGISTER_FILE_TYPE(GOWRMeshGpuHandler);
ONYX_REGISTER_FILE_TYPE(GOWRModelInstanceHandler);
ONYX_REGISTER_FILE_TYPE(GOWRTextureHandler);
ONYX_REGISTER_FILE_TYPE(GOWRRigHandler);

// ── GOWR Shader Viewer ────────────────────────────────────────────────────

class GOWRShaderViewer : public Viewers::IDocumentContent {
public:
    GOWRShaderViewer(const std::string& name, std::unique_ptr<GOWRShaderData> data)
        : m_name(name), m_data(std::move(data)) {}

    std::string GetName() const override { return "Shader: " + m_name; }

    void Draw() override {
        if (!m_data) {
            ImGui::TextDisabled("Failed to parse shader");
            return;
        }

        if (!ImGui::BeginTabBar("##shader_tabs")) return;

        if (ImGui::BeginTabItem("Container")) {
            // The container pass bails early on a few conditions; keeping
            // it in a lambda lets those returns stand without skipping the
            // matching ImGui End* calls.
            [&] {

        // ── Onyx Header ─────────────────────────────────────────────────
        ImGui::SeparatorText("Onyx Shader Header");
        if (ImGui::BeginTable("##gowhdr", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field",  ImGuiTableColumnFlags_WidthFixed, 140);
            ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch);

            Row("Stage",    "%s (%s)", m_data->stageTag.c_str(), m_data->StageName());
            Row("Version",  "%u.%u",  m_data->formatVersion, m_data->subVersion);
            Row("DXBC Size","%u bytes (%.1f KB)", m_data->dxbcSize, m_data->dxbcSize / 1024.0f);
            Row("PSO Flags","0x%08X", m_data->psoFlags);
            Row("Variant",  "0x%08X", m_data->variantId);

            ImGui::EndTable();
        }

        if (!m_data->hasDxbc) {
            ImGui::TextColored(ImVec4(1,0.5f,0.5f,1), "No valid DXBC container found.");
            return;
        }

        // ── Debug path (ILDN) ──────────────────────────────────────────
        if (!m_data->debugPath.empty()) {
            ImGui::Spacing();
            ImGui::SeparatorText("Build Path (ILDN)");
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
            ImGui::TextWrapped("%s", m_data->debugPath.c_str());
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                ImGui::SetClipboardText(m_data->debugPath.c_str());
        }

        // ── DXIL Info ──────────────────────────────────────────────────
        if (m_data->dxil.valid) {
            ImGui::Spacing();
            ImGui::SeparatorText("DXIL Payload");
            if (ImGui::BeginTable("##dxil", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Field",  ImGuiTableColumnFlags_WidthFixed, 140);
                ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch);
                Row("Shader Model", "%u.%u", m_data->dxil.majorVersion, m_data->dxil.minorVersion);
                Row("Bitcode Size", "%u bytes", m_data->dxil.bitcodeSize);
                ImGui::EndTable();
            }
        }

        // ── Chunks ─────────────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::SeparatorText("DXBC Chunks");
        if (ImGui::BeginTable("##chunks", 3,
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Sortable))
        {
            ImGui::TableSetupColumn("FourCC",  ImGuiTableColumnFlags_WidthFixed,  60);
            ImGui::TableSetupColumn("Offset",  ImGuiTableColumnFlags_WidthFixed,  80);
            ImGui::TableSetupColumn("Size",    ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableHeadersRow();

            for (const auto& c : m_data->chunks) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", c.tag);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("0x%04X", c.offset);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%u", c.size);
            }
            ImGui::EndTable();
        }

        // ── Input Signature ────────────────────────────────────────────
        DrawSignature("Input Signature (ISG1)", m_data->inputs);

        // ── Output Signature ───────────────────────────────────────────
        DrawSignature("Output Signature (OSG1)", m_data->outputs);

        // ── Patch Signature ────────────────────────────────────────────
        if (!m_data->patch.empty()) {
            DrawSignature("Patch Signature (PSG1)", m_data->patch);
        }

        // ── Statistics ─────────────────────────────────────────────────
        if (m_data->stats.valid) {
            ImGui::Spacing();
            ImGui::SeparatorText("Statistics (STAT)");
            if (ImGui::BeginTable("##stats", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthFixed, 160);
                ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch);
                Row("Instructions",    "%u", m_data->stats.instructionCount);
                Row("Temp Registers",  "%u", m_data->stats.tempRegisterCount);
                Row("Float Ops",       "%u", m_data->stats.floatOps);
                Row("Int Ops",         "%u", m_data->stats.intOps);
                Row("UInt Ops",        "%u", m_data->stats.uintOps);
                Row("Texture Ops",     "%u", m_data->stats.textureOps);
                ImGui::EndTable();
            }
        }

        // ── Shader Hash ────────────────────────────────────────────────
        if (m_data->hasHash) {
            ImGui::Spacing();
            ImGui::SeparatorText("Shader Hash");
            char hashStr[41] = {};
            for (int i = 0; i < 16; i++)
                snprintf(hashStr + i * 2, 3, "%02x", m_data->shaderHash[i]);
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.5f, 1.0f), "%s", hashStr);
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                ImGui::SetClipboardText(hashStr);
        }
            }();
            ImGui::EndTabItem();
        }

        DrawDisassemblyTab();
        ImGui::EndTabBar();
    }

private:
    // -- Disassembly ------------------------------------------------------
    // DXIL is LLVM bitcode, so the text comes from dxcompiler.dll rather
    // than from anything we parse. It is produced on demand and cached:
    // a shader is usually opened to read one thing, not to be re-decoded
    // every frame.
    void DrawDisassemblyTab() {
        if (!ImGui::BeginTabItem("Disassembly")) return;

        if (!m_data->hasDxbc) {
            ImGui::TextDisabled("No DXBC container to disassemble.");
            ImGui::EndTabItem();
            return;
        }

        if (!m_disasmTried) {
            m_disasmTried = true;
            std::string text;
            if (DxilDisassembler::Disassemble(m_data->dxbc.data(),
                                              m_data->dxbc.size(),
                                              text, m_disasmError)) {
                m_code.SetText(std::move(text));
            }
        }

        if (m_code.Empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f),
                               "Disassembly unavailable");
            ImGui::Spacing();
            ImGui::TextWrapped("%s", m_disasmError.c_str());
            ImGui::Spacing();
            ImGui::TextDisabled("The Container tab still shows signatures, "
                                "chunks and resource info, which are parsed "
                                "natively and need no external tool.");
            if (ImGui::Button("Retry")) m_disasmTried = false;
            ImGui::EndTabItem();
            return;
        }

        m_code.Draw("disasm");
        ImGui::EndTabItem();
    }

    Onyx::Ui::CodeView m_code;
    std::string        m_disasmError;
    bool               m_disasmTried = false;

    std::string m_name;
    std::unique_ptr<GOWRShaderData> m_data;

    template<typename... Args>
    void Row(const char* key, const char* fmt, Args... args) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("%s", key);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text(fmt, args...);
    }

    void DrawSignature(const char* title, const std::vector<SignatureElement>& sig) {
        if (sig.empty()) return;
        ImGui::Spacing();
        ImGui::SeparatorText(title);
        if (ImGui::BeginTable("##sig", 5,
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerH))
        {
            ImGui::TableSetupColumn("Semantic",   ImGuiTableColumnFlags_WidthFixed, 180);
            ImGui::TableSetupColumn("Index",      ImGuiTableColumnFlags_WidthFixed,  50);
            ImGui::TableSetupColumn("Type",       ImGuiTableColumnFlags_WidthFixed,  60);
            ImGui::TableSetupColumn("Register",   ImGuiTableColumnFlags_WidthFixed,  60);
            ImGui::TableSetupColumn("Mask",       ImGuiTableColumnFlags_WidthFixed,  60);
            ImGui::TableHeadersRow();

            for (const auto& e : sig) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (e.systemValueType != 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "%s",
                                       GOWRShaderData::SystemValueName(e.systemValueType));
                } else {
                    ImGui::Text("%s", e.semanticName.c_str());
                }
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", e.semanticIndex);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", GOWRShaderData::ComponentTypeName(e.componentType));
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("r%u", e.registerIndex);
                ImGui::TableSetColumnIndex(4);
                std::string mask = GOWRShaderData::MaskString(e.mask);
                ImGui::Text("%s", mask.c_str());
            }
            ImGui::EndTable();
        }
    }
};

std::shared_ptr<Viewers::IDocumentContent> GOWRShaderHandler::CreateViewer(const AssetEntry& entry, AssetContainer& wad) {
    if (!wad.fileSource || entry.source.size == 0) return nullptr;

    auto file = std::make_shared<Vfs::SliceFile>(wad.fileSource, entry.source.offset, entry.source.size);
    auto data = GOWRShaderParse(file);
    if (!data) return nullptr;

    return std::make_shared<GOWRShaderViewer>(entry.name, std::move(data));
}

// Register shader handlers for all shader TypeIds.
//
// Each takes its module CATALOG KEY, not a TypeId: these lambdas run during
// static initialisation, before any module has registered its types, and a
// GOWR tree carries module-minted ids anyway. See GOWRShaderHandler.
static bool _reg_shader_vs = [] {
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<Onyx::GOWRShaderHandler>("gowr.shaderVertex"));
    return true;
}();
static bool _reg_shader_ps = [] {
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<Onyx::GOWRShaderHandler>("gowr.shaderPixel"));
    return true;
}();
static bool _reg_shader_ct = [] {
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<Onyx::GOWRShaderHandler>("gowr.shaderContainer"));
    return true;
}();
static bool _reg_shader_hs = [] {
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<Onyx::GOWRShaderHandler>("gowr.shaderHull"));
    return true;
}();
static bool _reg_shader_ds = [] {
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<Onyx::GOWRShaderHandler>("gowr.shaderDomain"));
    return true;
}();
static bool _reg_shader_cs = [] {
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<Onyx::GOWRShaderHandler>("gowr.shaderCompute"));
    return true;
}();
static bool _reg_shader_ls = [] {
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<Onyx::GOWRShaderHandler>("gowr.shaderLibrary"));
    return true;
}();

// -- Material viewer ---------------------------------------------------------
// Shows what a material actually is: the parameters it feeds its shader's
// constant buffer, the textures it pulls in with the role each one plays, and
// the shader permutations compiled for it.
class GOWRMaterialViewer : public Viewers::IDocumentContent {
public:
    GOWRMaterialViewer(std::string name, GOWRMaterial mat)
        : m_name(std::move(name)), m_mat(std::move(mat)) {}

    std::string GetName() const override { return "Material: " + m_name; }

    void Draw() override {
        ImGui::SeparatorText("Material");
        ImGui::Text("Hash");
        ImGui::SameLine(140);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.5f, 1.0f), "%016llX",
                           (unsigned long long)m_mat.hash);
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)m_mat.hash);
            ImGui::SetClipboardText(buf);
        }

        const auto textures = m_mat.Textures();
        const auto shaders  = m_mat.Shaders();

        if (!ImGui::BeginTabBar("##mat_tabs")) return;

        if (ImGui::BeginTabItem("Textures")) {
            if (textures.empty()) {
                ImGui::TextDisabled("No reference list resolved for this material.");
            } else if (ImGui::BeginTable("##tex", 3,
                    ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_BordersInnerH)) {
                ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthFixed, 140);
                ImGui::TableSetupColumn("Hash", ImGuiTableColumnFlags_WidthFixed, 150);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                for (const auto* t : textures) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    const bool known = t->role != TextureRole::Unknown;
                    ImGui::TextColored(known ? ImVec4(0.5f, 1.0f, 0.6f, 1.0f)
                                             : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                                       "%s", TextureRoleName(t->role));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%016llX", (unsigned long long)t->textureHash);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(t->name.c_str());
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Parameters")) {
            ImGui::TextDisabled("Names are stored hashed; the strings are not in "
                                "the file. The offset is where the value lands in "
                                "the shader constant buffer.");
            if (ImGui::BeginTable("##par", 4,
                    ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_BordersInnerH)) {
                ImGui::TableSetupColumn("Name hash", ImGuiTableColumnFlags_WidthFixed, 150);
                ImGui::TableSetupColumn("Type",      ImGuiTableColumnFlags_WidthFixed, 60);
                ImGui::TableSetupColumn("CB offset", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Value",     ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                for (const auto& p : m_mat.params) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%016llX", (unsigned long long)p.nameHash);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%04X", p.typeCode);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("0x%02X", p.cbufferOffset);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%g", p.value);
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shaders")) {
            ImGui::TextDisabled("%zu permutations, all named after this "
                                "material's hash.", shaders.size());
            for (const auto* s : shaders)
                ImGui::BulletText("%s", s->name.c_str());
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

private:
    std::string  m_name;
    GOWRMaterial m_mat;
};

std::shared_ptr<Viewers::IDocumentContent> GOWRMaterialHandler::CreateViewer(
        const AssetEntry& entry, AssetContainer& wad) {
    if (!wad.fileSource || entry.source.size == 0) return nullptr;

    std::vector<const AssetEntry*> flat;
    FlattenEntries(wad.entries, flat);

    auto matFile = std::make_shared<Vfs::SliceFile>(wad.fileSource, entry.source.offset, entry.source.size);
    const MaterialRefIndex refIndex = BuildMaterialRefIndex(wad, flat);
    auto it      = refIndex.find(MaterialHashKey(entry.name));
    auto refFile = (it != refIndex.end()) ? it->second : nullptr;

    GOWRMaterial mat;
    if (!GOWRMaterialParse(matFile, refFile, mat)) return nullptr;
    return std::make_shared<GOWRMaterialViewer>(entry.name, std::move(mat));
}

static bool _reg_material = [] {
    ::Onyx::Types::TypeRegistry::Get().RegisterByTypeId(
        std::make_unique<Onyx::GOWRMaterialHandler>());
    return true;
}();

} // namespace Onyx
