#pragma once
#include <Onyx/Vfs/IFile.h>
#include <memory>
#include <vector>
#include <cstdint>

namespace Onyx {

// Parses an MG_* (mesh-group) file.
//
// The MG owns the detail-level structure of a model. It declares parts; each
// part names the MESH submeshes that make up every one of its levels, together
// with the camera distance at which each level stops being used. It also owns
// the per-part bone palette that turns a vertex's local bone index into a
// global PROTO skeleton index.
//
// See docs/GoWRknk/Formats/Mesh.md for the layout and how it was established.
class GOWRMgParser {
public:
    struct Level {
        float                 maxDistance = 0.0f;  // used while distance < this
        std::vector<uint16_t> submeshes;           // 0, 1 or 2 MESH submesh indices
    };

    struct Part {
        uint16_t              boneRef = 0;   // root bone; meaningful for rigid parts
        bool                  rigid   = false;
        std::vector<Level>    levels;        // real levels only; the terminator is dropped
        bool                  culledAtRange = false;  // terminator said "draw nothing"
        std::vector<uint16_t> palette;       // local bone index -> global skeleton index
    };

    struct Data {
        std::vector<Part> parts;

        // submesh index -> (part, level), or -1 when a submesh is unreferenced.
        std::vector<int> partOfSubmesh;
        std::vector<int> levelOfSubmesh;

        int MaxLevelCount() const {
            int n = 0;
            for (const auto& p : parts)
                if ((int)p.levels.size() > n) n = (int)p.levels.size();
            return n;
        }
    };

    // Full parse. meshSubmeshCount sizes the submesh->part/level lookups.
    static bool Parse(std::shared_ptr<Vfs::IFile> mgFile,
                      uint32_t meshSubmeshCount,
                      Data& out);

    // Compatibility helper: flat submesh -> owning bone map, 0xFFFF when the
    // submesh is unreferenced or its part is skinned rather than rigid.
    static bool ParseParentBones(std::shared_ptr<Vfs::IFile> mgFile,
                                 uint32_t meshSubmeshCount,
                                 std::vector<uint16_t>& outParentBone);
};

} // namespace Onyx
