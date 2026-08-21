#include <glad/glad.h>  // Must be before anything that pulls GL types

#include "cli/RenderCommand.h"
#include "cli/HeadlessGL.h"
#include "core/harness/AssetHarness.h"

#include <Onyx/Domain/IAssetProfile.h>
#include <Onyx/Rendering/Camera.h>
#include <Onyx/Rendering/GridRenderer.h>
#include <Onyx/Rendering/SceneRenderer.h>
#include <Onyx/Rendering/ShaderManager.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Onyx::Cli {

namespace {

// ── Options ──────────────────────────────────────────────────────────────

struct ViewSpec {
    std::string                   label;
    bool                          useSnap = false;
    Rendering::CameraView         snap = Rendering::CameraView::Front;
};

struct Options {
    fs::path    archive;
    std::string entry;
    std::string gameHint;
    fs::path    out;
    fs::path    report;
    int         width  = 1280;
    int         height = 720;
    Rendering::ShadingMode shading = Rendering::ShadingMode::Textured;
    std::vector<ViewSpec>  views;
    bool        grid  = true;
    bool        bones = false;
};

bool ParseShading(const std::string& s, Rendering::ShadingMode& out) {
    if (s == "solid")        { out = Rendering::ShadingMode::Solid;        return true; }
    if (s == "matcap")       { out = Rendering::ShadingMode::Matcap;       return true; }
    if (s == "textured")     { out = Rendering::ShadingMode::Textured;     return true; }
    if (s == "wireframe")    { out = Rendering::ShadingMode::Wireframe;    return true; }
    if (s == "texturedwire") { out = Rendering::ShadingMode::TexturedWire; return true; }
    return false;
}

bool ParseView(const std::string& s, ViewSpec& out) {
    using CV = Rendering::CameraView;
    // "iso" is the camera's own default orbit (yaw 45°, pitch 15°) — the same
    // angle the viewport opens at — so it is expressed as "no snap" rather
    // than as a CameraView, which has no isometric member.
    if (s == "iso")    { out = {"iso",    false, CV::Front};  return true; }
    if (s == "front")  { out = {"front",  true,  CV::Front};  return true; }
    if (s == "back")   { out = {"back",   true,  CV::Back};   return true; }
    if (s == "left")   { out = {"left",   true,  CV::Left};   return true; }
    if (s == "right")  { out = {"right",  true,  CV::Right};  return true; }
    if (s == "top")    { out = {"top",    true,  CV::Top};    return true; }
    if (s == "bottom") { out = {"bottom", true,  CV::Bottom}; return true; }
    return false;
}

bool ParseSize(const std::string& s, int& w, int& h) {
    auto x = s.find_first_of("xX");
    if (x == std::string::npos) return false;
    try {
        w = std::stoi(s.substr(0, x));
        h = std::stoi(s.substr(x + 1));
    } catch (...) { return false; }
    return w > 0 && h > 0 && w <= 8192 && h <= 8192;
}

// ── Minimal JSON emitter ─────────────────────────────────────────────────
// Hand-rolled because nlohmann/json is fetched only for the test target; the
// app binary has no JSON dependency and this report does not justify adding
// one. The shape is flat and fully controlled here.

std::string JsonEscape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    o += buf;
                } else {
                    o += (char)c;
                }
        }
    }
    return o;
}

std::string JsonStr(const std::string& s) { return "\"" + JsonEscape(s) + "\""; }
const char* JsonBool(bool b) { return b ? "true" : "false"; }

const char* BlendName(Parsers::BlendMode m) {
    switch (m) {
        case Parsers::BlendMode::Normal:      return "normal";
        case Parsers::BlendMode::Additive:    return "additive";
        case Parsers::BlendMode::Subtractive: return "subtractive";
        case Parsers::BlendMode::EnvMap:      return "envmap";
    }
    return "unknown";
}

/// Mirrors the filter in SceneRenderer::Build — parts with no geometry never
/// become batches. Kept here so the report can name each batch's materialId;
/// if the two ever drift, the size check below reports `null` rather than
/// silently pairing a batch with the wrong material.
std::vector<const Parsers::MeshPart*> DrawableParts(const Parsers::SceneData& scene) {
    std::vector<const Parsers::MeshPart*> out;
    for (const auto& p : scene.meshParts)
        if (!p.vertices.empty() && !p.indices.empty())
            out.push_back(&p);
    return out;
}

bool WriteReport(const fs::path& path,
                 const Options& opt,
                 const std::string& profileName,
                 const Parsers::SceneData& scene,
                 Rendering::SceneRenderer& renderer,
                 const std::vector<fs::path>& images,
                 std::string& err)
{
    if (path.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
    }
    std::ofstream f(path);
    if (!f) { err = "cannot write report: " + path.string(); return false; }

    const auto& batches = renderer.GetBatches();
    const auto  parts   = DrawableParts(scene);
    const bool  aligned = parts.size() == batches.size();

    const auto bounds = renderer.GetBounds();

    int noDiffuse = 0;
    for (const auto& b : batches)
        if (b.texture0 == 0) ++noDiffuse;

    f << "{\n";
    f << "  \"archive\": "  << JsonStr(opt.archive.string()) << ",\n";
    f << "  \"entry\": "    << JsonStr(opt.entry) << ",\n";
    f << "  \"profile\": "  << JsonStr(profileName) << ",\n";
    f << "  \"batchPartAlignment\": " << JsonBool(aligned) << ",\n";

    f << "  \"scene\": {\n";
    f << "    \"meshParts\": "   << scene.meshParts.size() << ",\n";
    f << "    \"drawableParts\": " << parts.size() << ",\n";
    f << "    \"materials\": "   << scene.materials.size() << ",\n";
    f << "    \"hasSkeleton\": " << JsonBool(scene.HasSkeleton()) << ",\n";
    f << "    \"joints\": "      << (scene.skeleton ? scene.skeleton->joints.size() : 0u) << ",\n";
    f << "    \"isSky\": "       << JsonBool(scene.isSky) << ",\n";
    f << "    \"pbrLayers\": "   << JsonBool(scene.pbrLayers) << ",\n";
    f << "    \"boundsMin\": [" << bounds.min.x << ", " << bounds.min.y << ", " << bounds.min.z << "],\n";
    f << "    \"boundsMax\": [" << bounds.max.x << ", " << bounds.max.y << ", " << bounds.max.z << "],\n";
    f << "    \"radius\": "      << bounds.Radius() << "\n";
    f << "  },\n";

    f << "  \"totals\": {\n";
    f << "    \"batches\": "          << batches.size() << ",\n";
    f << "    \"vertices\": "         << renderer.GetTotalVertices() << ",\n";
    f << "    \"triangles\": "        << renderer.GetTotalTriangles() << ",\n";
    f << "    \"batchesNoDiffuse\": " << noDiffuse << "\n";
    f << "  },\n";

    f << "  \"batches\": [\n";
    for (size_t i = 0; i < batches.size(); ++i) {
        const auto& b = batches[i];
        f << "    {\n";
        f << "      \"index\": " << i << ",\n";
        f << "      \"name\": "  << JsonStr(b.name) << ",\n";
        if (aligned) f << "      \"materialId\": " << parts[i]->materialId << ",\n";
        else         f << "      \"materialId\": null,\n";
        f << "      \"textureLayer\": " << b.textureLayer << ",\n";
        f << "      \"vertices\": "     << b.vertexCount << ",\n";
        f << "      \"triangles\": "    << b.triangleCount << ",\n";
        f << "      \"blendMode\": "    << JsonStr(BlendName(b.blendMode)) << ",\n";
        f << "      \"isSky\": "        << JsonBool(b.isSky) << ",\n";
        f << "      \"hasSkeleton\": "  << JsonBool(b.hasSkeleton) << ",\n";
        f << "      \"jointMapSize\": " << b.jointMap.size() << ",\n";
        f << "      \"meshHash\": "     << JsonStr([&]{
                std::ostringstream h; h << "0x" << std::hex << b.meshHash; return h.str(); }()) << ",\n";
        // The GL names below are what the draw call will actually bind. A
        // zero means the sampler falls back to whatever the shader does with
        // no texture — this is the field that answers "which meshes render
        // without a material".
        f << "      \"gl\": { \"diffuse\": " << b.texture0
          << ", \"envmap\": "  << b.texture1
          << ", \"normal\": "  << b.texNormal
          << ", \"ao\": "      << b.texAO
          << ", \"gloss\": "   << b.texGloss
          << ", \"scatter\": " << b.texScatter << " }\n";
        f << "    }" << (i + 1 < batches.size() ? "," : "") << "\n";
    }
    f << "  ],\n";

    f << "  \"materials\": [\n";
    for (size_t i = 0; i < scene.materials.size(); ++i) {
        const auto& m = scene.materials[i];
        f << "    { \"id\": " << i << ", \"layers\": [";
        for (size_t j = 0; j < m.layers.size(); ++j) {
            f << "{ \"index\": " << j
              << ", \"texture\": " << JsonStr(m.layers[j].textureName)
              << ", \"hasTexture\": " << JsonBool(m.layers[j].hasTexture)
              << ", \"blendMode\": " << JsonStr(BlendName(m.layers[j].blendMode))
              << " }" << (j + 1 < m.layers.size() ? ", " : "");
        }
        f << "] }" << (i + 1 < scene.materials.size() ? "," : "") << "\n";
    }
    f << "  ],\n";

    f << "  \"images\": [";
    for (size_t i = 0; i < images.size(); ++i)
        f << JsonStr(images[i].string()) << (i + 1 < images.size() ? ", " : "");
    f << "]\n";
    f << "}\n";

    return (bool)f;
}

} // namespace

void PrintRenderHelp() {
    std::cout
        << "Usage: GoWTool render <archive> <entry> [options]\n\n"
        << "Draws the asset offscreen through the same SceneRenderer the app's\n"
        << "viewport uses, and writes a PNG.\n\n"
        << "Options:\n"
        << "  --out <file.png>     Output image (default: <entry>.png)\n"
        << "  --game <hint>        gow2 | gowr | ragnarok (default: auto-detect)\n"
        << "  --size <WxH>         Frame size (default: 1280x720)\n"
        << "  --shading <mode>     solid | matcap | textured | wireframe |\n"
        << "                       texturedwire (default: textured)\n"
        << "  --view <name>        iso | front | back | left | right | top |\n"
        << "                       bottom (default: iso; repeatable)\n"
        << "  --angles             Shorthand for --view iso front right top\n"
        << "  --report <file.json> Per-batch report: geometry, material, and the\n"
        << "                       GL texture names the draw call binds\n"
        << "  --no-grid            Omit the ground grid\n"
        << "  --bones              Draw the skeleton overlay\n\n"
        << "With more than one view, each image gets a .<view> suffix.\n\n"
        << "Example:\n"
        << "  GoWTool render GOW.wad goProtoHeroA00 --angles \\\n"
        << "      --out shots/hero.png --report shots/hero.json\n";
}

int RunRenderCommand(const std::vector<std::string>& args) {
    Options opt;

    // args[0] == "render"
    if (args.size() < 3) { PrintRenderHelp(); return 2; }
    opt.archive = args[1];
    opt.entry   = args[2];

    auto needValue = [&](size_t& i, const char* flag) -> const std::string* {
        if (i + 1 >= args.size()) {
            std::cerr << "[render] " << flag << " needs a value\n";
            return nullptr;
        }
        return &args[++i];
    };

    for (size_t i = 3; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (a == "--out") {
            auto* v = needValue(i, "--out"); if (!v) return 2;
            opt.out = *v;
        } else if (a == "--game") {
            auto* v = needValue(i, "--game"); if (!v) return 2;
            opt.gameHint = *v;
        } else if (a == "--report") {
            auto* v = needValue(i, "--report"); if (!v) return 2;
            opt.report = *v;
        } else if (a == "--size") {
            auto* v = needValue(i, "--size"); if (!v) return 2;
            if (!ParseSize(*v, opt.width, opt.height)) {
                std::cerr << "[render] bad --size '" << *v << "' (expected WxH, max 8192)\n";
                return 2;
            }
        } else if (a == "--shading") {
            auto* v = needValue(i, "--shading"); if (!v) return 2;
            if (!ParseShading(*v, opt.shading)) {
                std::cerr << "[render] unknown --shading '" << *v << "'\n";
                return 2;
            }
        } else if (a == "--view") {
            auto* v = needValue(i, "--view"); if (!v) return 2;
            ViewSpec vs;
            if (!ParseView(*v, vs)) {
                std::cerr << "[render] unknown --view '" << *v << "'\n";
                return 2;
            }
            opt.views.push_back(vs);
        } else if (a == "--angles") {
            // One angle hides a lot: a vertex explosion along the view axis is
            // invisible head-on, and a collapsed rig reads as "fine" from the
            // front. Four spreads catch it.
            for (const char* n : {"iso", "front", "right", "top"}) {
                ViewSpec vs; ParseView(n, vs); opt.views.push_back(vs);
            }
        } else if (a == "--no-grid") {
            opt.grid = false;
        } else if (a == "--bones") {
            opt.bones = true;
        } else if (a == "-h" || a == "--help") {
            PrintRenderHelp();
            return 0;
        } else {
            std::cerr << "[render] unknown option '" << a << "'\n";
            PrintRenderHelp();
            return 2;
        }
    }

    if (opt.views.empty()) {
        ViewSpec vs; ParseView("iso", vs); opt.views.push_back(vs);
    }
    if (opt.out.empty()) opt.out = opt.entry + ".png";

    // ── Parse ────────────────────────────────────────────────────────────
    Harness::LoadRequest req{opt.archive, opt.entry, opt.gameHint};
    Harness::LoadResult  load;
    if (!Harness::Load(req, load)) {
        std::cerr << "[render] " << load.error << "\n";
        if (!load.container.entries.empty() && !load.entry) {
            std::vector<std::string> names;
            Harness::CollectEntryNames(load.container.entries, names);
            std::sort(names.begin(), names.end());
            std::cerr << "[render] " << names.size() << " entries in this container; first 40:\n";
            for (size_t i = 0; i < names.size() && i < 40; ++i)
                std::cerr << "    " << names[i] << "\n";
        }
        return 1;
    }

    const std::string profileName =
        load.container.profile ? load.container.profile->GetName() : "(none)";
    std::cout << "[render] profile: " << profileName << "\n";
    Harness::PrintSceneStats(*load.scene, std::cout);

    if (load.scene->IsEmpty()) {
        std::cerr << "[render] scene has no mesh parts — nothing to draw.\n";
        return 3;
    }

    // ── GPU ──────────────────────────────────────────────────────────────
    HeadlessGL gl;
    std::string err;
    if (!gl.Init(err)) {
        std::cerr << "[render] " << err << "\n";
        return 4;
    }

    Rendering::SceneRenderer renderer;
    renderer.Build(*load.scene);
    if (renderer.IsEmpty()) {
        std::cerr << "[render] SceneRenderer produced no batches.\n";
        return 3;
    }

    Rendering::GridRenderer grid;
    grid.Initialize();

    Rendering::Camera camera;

    std::vector<fs::path> written;
    const bool suffixed = opt.views.size() > 1;

    for (const auto& view : opt.views) {
        // FocusOn resets the orbit and clears any pending tween, so it has to
        // come before the snap, not after.
        camera.FocusOn(renderer.GetBounds());
        if (view.useSnap) {
            camera.SnapToView(view.snap);
            // The snap is a 0.25s tween built for the UI; one oversized step
            // lands it on the final orientation with no frames in between.
            camera.UpdateAnimation(1.0f);
        }

        if (!gl.BeginFrame(opt.width, opt.height, err)) {
            std::cerr << "[render] " << err << "\n";
            return 4;
        }

        const float aspect = (float)opt.width / (float)opt.height;
        const glm::mat4 v = camera.GetViewMatrix();
        const glm::mat4 p = camera.GetProjectionMatrix(aspect);

        // Same order as Viewport3D::Draw. AppConfig is not loaded in CLI mode,
        // so this uses the viewport's own fallback gradient.
        Rendering::SceneRenderer::RenderBackground(glm::vec3(0.18f, 0.18f, 0.22f),
                                                   glm::vec3(0.08f, 0.08f, 0.10f));
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glClear(GL_DEPTH_BUFFER_BIT);

        if (renderer.HasSky())
            renderer.RenderSky(v, p, opt.shading);

        renderer.Render(v, p, opt.shading, opt.width, opt.height);

        if (opt.bones && renderer.HasSkeleton())
            renderer.RenderSkeleton(v, p);

        if (opt.grid) {
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            grid.Draw(v, p, camera.GetPosition(),
                      glm::vec4(0.35f, 0.35f, 0.35f, 0.5f), 1.0f);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
        }

        std::vector<uint8_t> pixels;
        if (!gl.EndFrame(pixels, err)) {
            std::cerr << "[render] " << err << "\n";
            return 4;
        }

        fs::path target = opt.out;
        if (suffixed) {
            fs::path stem = opt.out;
            stem.replace_extension();
            target = stem.string() + "." + view.label + opt.out.extension().string();
        }
        if (!HeadlessGL::WritePng(target, opt.width, opt.height, pixels, err)) {
            std::cerr << "[render] " << err << "\n";
            return 5;
        }
        std::cout << "[render] wrote " << target.string()
                  << " (" << opt.width << "x" << opt.height << ", " << view.label << ")\n";
        written.push_back(target);
    }

    if (!opt.report.empty()) {
        if (!WriteReport(opt.report, opt, profileName, *load.scene, renderer, written, err)) {
            std::cerr << "[render] " << err << "\n";
            return 5;
        }
        std::cout << "[render] wrote " << opt.report.string() << "\n";
    }

    // Batches whose diffuse sampler will bind zero — the headline number this
    // command exists to produce.
    int noDiffuse = 0;
    for (const auto& b : renderer.GetBatches())
        if (b.texture0 == 0) ++noDiffuse;
    std::cout << "[render] " << renderer.GetBatches().size() << " batches, "
              << noDiffuse << " without a diffuse texture\n";

    // The GPU resources must die while the context is still current.
    renderer.Clear();
    return 0;
}

} // namespace Onyx::Cli
